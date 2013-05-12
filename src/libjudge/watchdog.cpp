#include <memory>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <limits>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/event.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <windows.h>
extern "C" {
	#include <ntndk.h>
}
#include <process.h>
#include "watchdog.hpp"
#include "exception.hpp"

using namespace std;
using stlsoft::lock_scope;
using winstl::event;
using winstl::thread_mutex;

namespace judge {

watchdog::context_base::context_base(
	const shared_ptr<judge::process> &ps,
	const shared_ptr<judge::job_object> &job,
	uint32_t time_limit_ms)
	: process_(ps)
	, job_object_(job)
	, time_limit_ms_(time_limit_ms)
{}

const shared_ptr<judge::process> watchdog::context_base::process()
{
	return process_;
}

const shared_ptr<judge::job_object> watchdog::context_base::job_object()
{
	return job_object_;
}

uint32_t watchdog::context_base::time_limit_ms()
{
	return time_limit_ms_;
}

watchdog::watchdog()
	: meta_event_(false, false)
	, exiting_(false)
	, thread_handle_(_create_thread())
{}

watchdog::~watchdog()
{
	::CloseHandle(thread_handle_);
}

void watchdog::rundown()
{
	exiting_ = true;
	meta_event_.set();
	::WaitForSingleObject(thread_handle_, INFINITE);
}

HANDLE watchdog::_create_thread()
{
	HANDLE result = reinterpret_cast<HANDLE>(
		::_beginthreadex(nullptr, 0, _thread_entry, this, 0, nullptr));
	if (!result) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}
	return result;
}

unsigned int __stdcall watchdog::_thread_entry(void *param)
{
	watchdog &wd = *reinterpret_cast<watchdog *>(param);
	vector<HANDLE> wait_handles;

	while (true) {
		wait_handles.clear();
		wait_handles.push_back(wd.meta_event_.handle());
		bool has_timeout = false;
		uint32_t timeout_value = numeric_limits<uint32_t>::max();

		{
			lock_scope<thread_mutex> lock(wd.contexts_mutex_);
			if (wd.contexts_.empty()) {
				if (wd.exiting_)
					return 0;
			} else {
				for_each(wd.contexts_.begin(), wd.contexts_.end(),
					[&wait_handles, &has_timeout, &timeout_value]
					(const std::shared_ptr<context_base> &context)->void
				{
					process &ps = *context->process();
					wait_handles.push_back(ps.handle());
					const uint32_t watchdog_jitter_ms = 10;
					uint32_t time_limit = context->time_limit_ms();
					uint32_t alive_time = ps.alive_time_ms();
					if (alive_time > time_limit) {
						try {
							context->job_object()->terminate(ERROR_NOT_ENOUGH_QUOTA);
						} catch (const win32_exception &) {
						}
					} else {
						uint32_t wait_time = time_limit - alive_time + watchdog_jitter_ms;
						has_timeout = true;
						timeout_value = min(timeout_value, wait_time);
					}
				});
			}
		}

		DWORD result = ::WaitForMultipleObjects(static_cast<DWORD>(wait_handles.size()),
			wait_handles.data(), FALSE, has_timeout ? static_cast<DWORD>(timeout_value) : INFINITE);
		if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + wait_handles.size()) {
			if (result != WAIT_OBJECT_0) {
				size_t index = static_cast<size_t>(result - (WAIT_OBJECT_0 + 1));
				std::shared_ptr<context_base> context;
				{
					// wd.contexts_[index] is correct because watchdog::add only appends at the back
					lock_scope<thread_mutex> lock(wd.contexts_mutex_);
					context = move(wd.contexts_[index]);
					wd.contexts_[index] = move(wd.contexts_.back());
					wd.contexts_.pop_back();
				}
				try {
					context->invoke();
				} catch (...) {
					assert(!"uncaught exception");
				}
			}
		} else if (result == WAIT_FAILED) {
			throw win32_exception(::GetLastError());
		}
	}
}

}
