#include <memory>
#include <string>
#include <fstream>
#include <algorithm>
#include <winstl/synch/event.hpp>
#include <winstl/synch/wait_functions.hpp>
#include <windows.h>
#include "job_object.hpp"
#include "process_monitor.hpp"
#include "watchdog.hpp"
#include "bunny.hpp"
#include "env.hpp"
#include "pool.hpp"
#include "exception.hpp"

using namespace std;
using winstl::wait_for_multiple_objects;
using winstl::path_a;

namespace judge {

struct bunny::context {
	context()
		: pm_event(true, false)
		, wd_event(true, false)
	{}

	winstl::event pm_event;
	winstl::event wd_event;
	process_monitor::result result;
};

bunny::bunny(env &env, bool trusted,
	const string &executable_path, const string &command_line, const path_a &current_dir,
	HANDLE stdin_handle, HANDLE stdout_handle, HANDLE stderr_handle, const judge_limit &limit)
	: job_(new job_object)
	, context_(new context)
	, limit_(limit)
	, section_size_(_get_section_size(executable_path))
{
	if (section_size_ == 0) {
		if (limit.memory_limit_kb) {
			context_->result.exit_status = -1;
			context_->pm_event.set();
			context_->wd_event.set();
			return;
		}
	} else if (limit.memory_limit_kb &&
		section_size_ > static_cast<uint64_t>(limit.memory_limit_kb) * 1024) {
		context_->result.exit_status = ERROR_NOT_ENOUGH_QUOTA;
		context_->pm_event.set();
		context_->wd_event.set();
		return;
	}

	_tune_job_object();

	if (trusted) {
		auto pp = env.create_process_trusted(executable_path, command_line, current_dir,
			stdin_handle, stdout_handle, stderr_handle);
		process_ = pp.first;
		thread_suspension_ = pp.second;
	} else {
		auto pp = env.create_process(executable_path, command_line, current_dir,
			stdin_handle, stdout_handle, stderr_handle);
		process_ = pp.first;
		thread_suspension_ = pp.second;
	}

	job_->assign(process_->handle());
	
	const shared_ptr<bunny::context> &context = context_;
	env.pool().process_monitor().add(process_, job_, [context](process_monitor::result &&result)->void {
		context->result = move(result);
		context->pm_event.set();
	});

	if (limit.time_limit_ms) {
		env.pool().watchdog().add(process_, job_, limit.time_limit_ms, [context]()->void {
			context->wd_event.set();
		});
	} else {
		context->wd_event.set();
	}
}

void bunny::_tune_job_object()
{
	{
		job_object::limits_info limits = job_->limits();
		if (limit_.memory_limit_kb != 0) {
			const SIZE_T memory_limit_jitter_bytes = 524288;
			uint64_t actual = static_cast<uint64_t>(limit_.memory_limit_kb) * 1024 + memory_limit_jitter_bytes;
			if (actual > MAXSIZE_T) {
				throw judge_exception(JSTATUS_PARAMETER_TOO_LARGE);
			}
			limits.ProcessMemoryLimit = static_cast<SIZE_T>(actual);
			limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
		}
		if (limit_.active_process_limit != 0) {
			limits.BasicLimitInformation.ActiveProcessLimit = static_cast<DWORD>(limit_.active_process_limit);
			limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
		}
		limits.BasicLimitInformation.LimitFlags
			|= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
			| JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		limits.commit();
	}
	{
		job_object::ui_restrictions_info ui_res = job_->ui_restrictions();
		ui_res.UIRestrictionsClass
			|= JOB_OBJECT_UILIMIT_HANDLES
			| JOB_OBJECT_UILIMIT_READCLIPBOARD
			| JOB_OBJECT_UILIMIT_WRITECLIPBOARD
			| JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS
			| JOB_OBJECT_UILIMIT_DISPLAYSETTINGS
			| JOB_OBJECT_UILIMIT_GLOBALATOMS
			| JOB_OBJECT_UILIMIT_DESKTOP
			| JOB_OBJECT_UILIMIT_EXITWINDOWS;
		ui_res.commit();
	}
}

uint64_t bunny::_get_section_size(const std::string &executable_path)
{
	ifstream in(executable_path.c_str(), ios::binary);
	if (!in.is_open())
		return 0;
	
	IMAGE_DOS_HEADER dos_header;
	if (!in.read(reinterpret_cast<char *>(&dos_header), sizeof(dos_header)))
		return 0;
	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		return 0;
	if (!in.seekg(dos_header.e_lfanew))
		return 0;

	DWORD signature;
	if (!in.read(reinterpret_cast<char *>(&signature), sizeof(signature)))
		return 0;
	if (signature != IMAGE_NT_SIGNATURE)
		return 0;
	IMAGE_FILE_HEADER file_header;
	if (!in.read(reinterpret_cast<char *>(&file_header), sizeof(file_header)))
		return 0;
	DWORD alignment;
	if (file_header.Machine == IMAGE_FILE_MACHINE_I386
		&& file_header.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)) {
		IMAGE_OPTIONAL_HEADER32 opt_header;
		if (!in.read(reinterpret_cast<char *>(&opt_header), sizeof(opt_header)))
			return 0;
		if (opt_header.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			return 0;
		alignment = opt_header.SectionAlignment;
	} else if (file_header.Machine == IMAGE_FILE_MACHINE_AMD64
		&& file_header.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64)) {
		IMAGE_OPTIONAL_HEADER64 opt_header;
		if (!in.read(reinterpret_cast<char *>(&opt_header), sizeof(opt_header)))
			return 0;
		if (opt_header.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			return 0;
		alignment = opt_header.SectionAlignment;
	} else {
		return 0;
	}
	
	uint64_t result = 0;
	for (int i = 0; i < file_header.NumberOfSections; ++i) {
		IMAGE_SECTION_HEADER section_header;
		if (!in.read(reinterpret_cast<char *>(&section_header), sizeof(section_header)))
			return 0;
		DWORD virtual_size = section_header.Misc.VirtualSize;
		virtual_size = (virtual_size + (alignment - 1)) & ~(alignment - 1);
		result += virtual_size;
	}

	return result;
}

void bunny::start()
{
	thread_suspension_.reset();
}

bunny::result bunny::wait()
{
	bunny::result result = {0};
	DWORD wait_result = wait_for_multiple_objects(context_->pm_event, context_->wd_event, true, INFINITE);
	if (wait_result == WAIT_FAILED) {
		throw win32_exception(::GetLastError());
	}

	result.exit_code = context_->result.exit_status;
	if (process_) {
		result.time_usage_ms = process_->alive_time_ms();
		result.memory_usage_kb = process_->peak_memory_usage_kb();
		result.runtime_error = RUNTIME_UNKNOWN;
	} else {
		result.flag = max(result.flag, JUDGE_RUNTIME_ERROR);
		result.time_usage_ms = 0;
		result.memory_usage_kb = static_cast<uint32_t>(section_size_ / 1024);
		result.runtime_error = RUNTIME_START_FAILED;
	}

	if (limit_.memory_limit_kb && result.memory_usage_kb > limit_.memory_limit_kb) {
		result.flag = max(result.flag, JUDGE_MEMORY_LIMIT_EXCEEDED);
	} else if (limit_.time_limit_ms && result.time_usage_ms > limit_.time_limit_ms) {
		result.flag = max(result.flag, JUDGE_TIME_LIMIT_EXCEEDED);
	} else if (context_->result.has_exception) {
		result.flag = max(result.flag, JUDGE_RUNTIME_ERROR);
		switch (context_->result.exception.ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			switch (context_->result.exception.ExceptionInformation[0]) {
			case 0:
				result.runtime_error = RUNTIME_READ_ACCESS_VIOLATION;
				break;
			case 1:
				result.runtime_error = RUNTIME_WRITE_ACCESS_VIOLATION;
				break;
			case 8:
				result.runtime_error = RUNTIME_EXECUTE_ACCESS_VIOLATION;
				break;
			}
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			result.runtime_error = RUNTIME_DIVIDE_BY_ZERO;
			break;
		case EXCEPTION_STACK_OVERFLOW:
			result.runtime_error = RUNTIME_STACK_OVERFLOW;
			break;
		}
	} else {
		result.flag = max(result.flag, JUDGE_ACCEPTED);
	}

	return result;
}

}
