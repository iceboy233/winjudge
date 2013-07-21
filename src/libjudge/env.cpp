#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <winstl/filesystem/path.hpp>
#include <windows.h>
#include <aclapi.h>
#include "env.hpp"
#include "pool.hpp"
#include "process.hpp"
#include "thread_suspension.hpp"
#include "exception.hpp"
#include "desktop.hpp"
#include "window_station.hpp"
#include "random_string.hpp"
#include "proc_thread_attribute_list.hpp"

using namespace std;
using namespace judge;
using stlsoft::lock_scope;
using winstl::thread_mutex;
using winstl::path_a;

namespace {

class inherit_handle {
public:
	inherit_handle(HANDLE handle)
		: handle_(handle)
		, original_(handle ? _original(handle) : 0)
	{
		if (handle) {
			BOOL success = ::SetHandleInformation(handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
			if (!success) {
				throw win32_exception(::GetLastError());
			}
		}
	}

	~inherit_handle()
	{
		if (handle_ && !(original_ & HANDLE_FLAG_INHERIT)) {
			::SetHandleInformation(handle_, HANDLE_FLAG_INHERIT, original_);
		}
	}

private:
	DWORD _original(HANDLE handle)
	{
		DWORD flags;
		BOOL success = ::GetHandleInformation(handle, &flags);
		if (!success) {
			throw win32_exception(::GetLastError());
		}
		return flags;
	}

private:
	HANDLE handle_;
	DWORD original_;
};

class duplicate_handle {
public:
	duplicate_handle(HANDLE handle, uintptr_t &value)
		: source_(handle)
		, target_(handle ? reinterpret_cast<HANDLE>(value += 4) : NULL)
	{}

	HANDLE target()
	{
		return target_;
	}

	void operator()(HANDLE target_process)
	{
		if (source_) {
			HANDLE target;
			BOOL result = ::DuplicateHandle(::GetCurrentProcess(), source_,
				target_process, &target, 0, FALSE, DUPLICATE_SAME_ACCESS);
			if (!result) {
				throw win32_exception(::GetLastError());
			}
			if (target != target_) {
				throw judge_exception(JSTATUS_INCORRECT_HANDLE);
			}
		}
	}

private:
	HANDLE source_;
	HANDLE target_;
};

}

namespace judge {

env::env(const std::shared_ptr<judge::pool> &pool)
	: pool_(pool)
{}

judge::pool &env::pool()
{
	return *pool_;
}

pair<shared_ptr<process>, shared_ptr<thread_suspension> > env::create_process(
	const string &executable_path, const string &command_line, const path_a &current_dir,
	HANDLE stdin_handle, HANDLE stdout_handle, HANDLE stderr_handle)
{
	return create_process_trusted(executable_path, command_line, current_dir,
		stdin_handle, stdout_handle, stderr_handle);
}

pair<shared_ptr<process>, shared_ptr<thread_suspension> > env::create_process_trusted(
	const string &executable_path, const string &command_line, const path_a &current_dir,
	HANDLE stdin_handle, HANDLE stdout_handle, HANDLE stderr_handle)
{
	PROCESS_INFORMATION process_info;
	BOOL result;

	if (!proc_thread_attribute_list::is_supported()) {
		STARTUPINFOA startup_info = {0};
		startup_info.cb = sizeof(startup_info);
		startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;

		uintptr_t handle_value = 0;
		duplicate_handle stdin_h(stdin_handle, handle_value);
		duplicate_handle stdout_h(stdout_handle, handle_value);
		duplicate_handle stderr_h(stderr_handle, handle_value);
		startup_info.hStdInput = stdin_h.target();
		startup_info.hStdOutput = stdout_h.target();
		startup_info.hStdError = stderr_h.target();

		result = ::CreateProcessA(executable_path.empty() ? NULL : executable_path.c_str(),
			const_cast<LPSTR>(command_line.c_str()), NULL, NULL, FALSE,
			CREATE_BREAKAWAY_FROM_JOB | CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | CREATE_SUSPENDED,
			NULL, current_dir.c_str(), &startup_info, &process_info);

		if (result) {
			try {
				stdin_h(process_info.hProcess);
				stdout_h(process_info.hProcess);
				stderr_h(process_info.hProcess);
			} catch (...) {
				::TerminateProcess(process_info.hProcess, 1);
				::CloseHandle(process_info.hProcess);
				::CloseHandle(process_info.hThread);
				throw;
			}
		}

	} else {
		inherit_handle stdin_h(stdin_handle);
		inherit_handle stdout_h(stdout_handle);
		inherit_handle stderr_h(stderr_handle);

		proc_thread_attribute_list attr_list(1);
		vector<HANDLE> handle_list;
		if (stdin_handle) handle_list.push_back(stdin_handle);
		if (stdout_handle) handle_list.push_back(stdout_handle);
		if (stderr_handle) handle_list.push_back(stderr_handle);
		sort(handle_list.begin(), handle_list.end());
		handle_list.erase(unique(handle_list.begin(), handle_list.end()), handle_list.end());
		attr_list.update(PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handle_list.data(), handle_list.size() * sizeof(HANDLE));

		STARTUPINFOEXA info = {0};
		info.StartupInfo.cb = sizeof(info);
		info.StartupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
		info.StartupInfo.hStdInput = stdin_handle;
		info.StartupInfo.hStdOutput = stdout_handle;
		info.StartupInfo.hStdError = stderr_handle;
		info.lpAttributeList = attr_list.pointer();

		result = ::CreateProcessA(executable_path.empty() ? NULL : executable_path.c_str(),
			const_cast<LPSTR>(command_line.c_str()), NULL, NULL, TRUE,
			CREATE_BREAKAWAY_FROM_JOB | CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
			NULL, current_dir.c_str(), &info.StartupInfo, &process_info);
	}

	if (!result) {
		throw win32_exception(::GetLastError());
	}

	shared_ptr<process> p;
	try {
		p.reset(new process(process_info.hProcess));
	} catch (...) {
		::TerminateProcess(process_info.hProcess, 1);
		::CloseHandle(process_info.hProcess);
		::CloseHandle(process_info.hThread);
		throw;
	}

	shared_ptr<thread_suspension> t;
	try {
		t.reset(new thread_suspension(process_info.hThread));
	} catch (...) {
		::TerminateProcess(process_info.hProcess, 1);
		::CloseHandle(process_info.hThread);
		throw;
	}

	return make_pair(move(p), move(t));
}

restricted_env::restricted_env(const std::shared_ptr<judge::pool> &pool,
	const string &username, const string &password)
	: env(pool)
	, username_(username)
	, session_(username, password)
	, desktop_(random_string(desktop_name_length))
{
	vector<char> sid = session_.sid();
	window_station_.remove_ace_by_sid(sid);
	window_station_.add_allowed_ace(sid, user_object::allowed_ace(0,
		GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE));
	desktop_.add_allowed_ace(sid, user_object::allowed_ace(0,
		GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE));
}

pair<shared_ptr<process>, shared_ptr<thread_suspension> > restricted_env::create_process(
	const string &executable_path, const string &command_line, const path_a &current_dir,
	HANDLE stdin_handle, HANDLE stdout_handle, HANDLE stderr_handle)
{
	string desktop_name = window_station_.name() + "\\" + desktop_.name();
	PROCESS_INFORMATION process_info;
	BOOL result;

	if (!proc_thread_attribute_list::is_supported()) {
		STARTUPINFOA startup_info = {0};
		startup_info.cb = sizeof(startup_info);
		startup_info.lpDesktop = const_cast<LPSTR>(desktop_name.c_str());
		startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;

		uintptr_t handle_value = 0;
		duplicate_handle stdin_h(stdin_handle, handle_value);
		duplicate_handle stdout_h(stdout_handle, handle_value);
		duplicate_handle stderr_h(stderr_handle, handle_value);
		startup_info.hStdInput = stdin_h.target();
		startup_info.hStdOutput = stdout_h.target();
		startup_info.hStdError = stderr_h.target();

		result = ::CreateProcessAsUserA(session_.handle(), executable_path.empty() ? NULL : executable_path.c_str(),
			const_cast<LPSTR>(command_line.c_str()), NULL, NULL, FALSE,
			CREATE_BREAKAWAY_FROM_JOB | CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | CREATE_SUSPENDED,
			NULL, current_dir.c_str(), &startup_info, &process_info);

		if (result) {
			try {
				stdin_h(process_info.hProcess);
				stdout_h(process_info.hProcess);
				stderr_h(process_info.hProcess);
			} catch (...) {
				::TerminateProcess(process_info.hProcess, 1);
				::CloseHandle(process_info.hProcess);
				::CloseHandle(process_info.hThread);
				throw;
			}
		}

	} else {
		inherit_handle stdin_h(stdin_handle);
		inherit_handle stdout_h(stdout_handle);
		inherit_handle stderr_h(stderr_handle);

		proc_thread_attribute_list attr_list(1);
		vector<HANDLE> handle_list;
		if (stdin_handle) handle_list.push_back(stdin_handle);
		if (stdout_handle) handle_list.push_back(stdout_handle);
		if (stderr_handle) handle_list.push_back(stderr_handle);
		sort(handle_list.begin(), handle_list.end());
		handle_list.erase(unique(handle_list.begin(), handle_list.end()), handle_list.end());
		attr_list.update(PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handle_list.data(), handle_list.size() * sizeof(HANDLE));

		STARTUPINFOEXA info = {0};
		info.StartupInfo.cb = sizeof(info);
		info.StartupInfo.lpDesktop = const_cast<LPSTR>(desktop_name.c_str());
		info.StartupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
		info.StartupInfo.hStdInput = stdin_handle;
		info.StartupInfo.hStdOutput = stdout_handle;
		info.StartupInfo.hStdError = stderr_handle;
		info.lpAttributeList = attr_list.pointer();

		result = ::CreateProcessAsUserA(session_.handle(), executable_path.empty() ? NULL : executable_path.c_str(),
			const_cast<LPSTR>(command_line.c_str()), NULL, NULL, TRUE,
			CREATE_BREAKAWAY_FROM_JOB | CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
			NULL, current_dir.c_str(), &info.StartupInfo, &process_info);
	}

	if (!result) {
		throw win32_exception(::GetLastError());
	}

	shared_ptr<process> p;
	try {
		p.reset(new process(process_info.hProcess));
	} catch (...) {
		::TerminateProcess(process_info.hProcess, 1);
		::CloseHandle(process_info.hProcess);
		::CloseHandle(process_info.hThread);
		throw;
	}

	shared_ptr<thread_suspension> t;
	try {
		t.reset(new thread_suspension(process_info.hThread));
	} catch (...) {
		::TerminateProcess(process_info.hProcess, 1);
		::CloseHandle(process_info.hThread);
		throw;
	}

	return make_pair(move(p), move(t));
}

void restricted_env::grant_access(const winstl::path_a &path)
{
	LPSTR username = const_cast<LPSTR>(username_.c_str());
	DWORD result;
	HANDLE file = ::CreateFileA(path.c_str(), READ_CONTROL | WRITE_DAC,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		throw win32_exception(::GetLastError());
	}

	PACL old_dacl, new_dacl;
	PSECURITY_DESCRIPTOR sd;
	EXPLICIT_ACCESSA ea = {0};

	result = ::GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
		NULL, NULL, &old_dacl, NULL, &sd);
	if (result != ERROR_SUCCESS) {
		::CloseHandle(file);
		throw win32_exception(result);
	}

	ea.grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE;
	ea.grfAccessMode = GRANT_ACCESS;
	ea.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
	ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea.Trustee.ptstrName = username;
	result = ::SetEntriesInAclA(1, &ea, old_dacl, &new_dacl);
	if (result != ERROR_SUCCESS) {
		::LocalFree(sd);
		::CloseHandle(file);
		throw win32_exception(result);
	}

	result = ::SetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
		NULL, NULL, new_dacl, NULL);
	if (result != ERROR_SUCCESS) {
		::LocalFree(new_dacl);
		::LocalFree(sd);
		::CloseHandle(file);
		throw win32_exception(result);
	}

	::LocalFree(new_dacl);
	::LocalFree(sd);
	::CloseHandle(file);
}

}
