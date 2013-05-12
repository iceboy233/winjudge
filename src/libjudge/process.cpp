#include <cstdint>
#include <algorithm>
#include <windows.h>
#include <psapi.h>
#include <winstl/shims/conversion/to_uint64.hpp>
#include "process.hpp"
#include "exception.hpp"
#include "util.hpp"

using namespace std;
using stlsoft::to_uint64;

namespace judge {

process::process(HANDLE handle)
	: handle_(handle)
	, timer_started_(false)
	, processor_count_(util::get_processor_count())
{}

process::~process()
{
	::CloseHandle(handle_);
}

HANDLE process::handle()
{
	return handle_;
}

void process::terminate(int32_t exit_code)
{
	BOOL result = ::TerminateProcess(handle(), static_cast<UINT>(exit_code));
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

uint32_t process::id()
{
	DWORD result = ::GetProcessId(handle());
	return static_cast<uint32_t>(result);
}

void process::start_timer()
{
	initial_process_time_ = _process_time();
	initial_idle_time_ = util::get_idle_time();
	timer_started_ = true;
}

uint64_t process::_process_time()
{
	FILETIME creation_time, exit_time, kernel_time, user_time;
	BOOL result = ::GetProcessTimes(handle(), &creation_time, &exit_time, &kernel_time, &user_time);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
	
	return to_uint64(reinterpret_cast<ULARGE_INTEGER &>(kernel_time))
		+ to_uint64(reinterpret_cast<ULARGE_INTEGER &>(user_time));
}

uint32_t process::alive_time_ms()
{
	if (!timer_started_) {
		return 0;
	} else {
		return static_cast<uint32_t>(
			max(_process_time() - initial_process_time_,
				(util::get_idle_time() - initial_idle_time_) / processor_count_
				) / 10000);
	}
}

uint32_t process::peak_memory_usage_kb()
{
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);
	BOOL result = ::GetProcessMemoryInfo(handle(), &pmc, sizeof(pmc));
	if (!result) {
		throw win32_exception(::GetLastError());
	}

	return static_cast<uint32_t>(pmc.PeakPagefileUsage / 1024);
}

uint32_t process::virtual_protect(void *address, size_t length, uint32_t new_protect)
{
	DWORD old_protect;
	BOOL result = ::VirtualProtectEx(handle(), address, length, new_protect, &old_protect);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
	return old_protect;
}

void process::read_memory(const void *address, void *buffer, size_t length)
{
	BOOL result = ::ReadProcessMemory(handle(), address, buffer, length, NULL);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

void process::write_memory(void *address, const void *buffer, size_t length)
{
	BOOL result = ::WriteProcessMemory(handle(), address, buffer, length, NULL);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

}
