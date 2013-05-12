#include <memory>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <winstl/synch/event.hpp>
#include <windows.h>
#include <process.h>
#include "timer_queue.hpp"
#include "exception.hpp"
#include "util.hpp"

using namespace std;
using stlsoft::lock_scope;
using winstl::thread_mutex;

namespace judge {

timer_queue::timer_queue()
	: notify_event_(false, false)
	, exiting_(false)
	, thread_handle_(_create_thread())
{}

timer_queue::~timer_queue()
{
	::CloseHandle(thread_handle_);
}

void timer_queue::rundown()
{
	exiting_ = true;
	notify_event_.set();
	::WaitForSingleObject(thread_handle_, INFINITE);
}

timer_queue::context_base::context_base(unsigned delay_ms)
	: expire_(util::get_tick_count() + delay_ms)
{}

uint64_t timer_queue::context_base::expire()
{
	return expire_;
}

HANDLE timer_queue::_create_thread()
{
	HANDLE result = reinterpret_cast<HANDLE>(
		::_beginthreadex(nullptr, 0, _thread_entry, this, 0, nullptr));
	if (!result) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}
	return result;
}

unsigned int __stdcall timer_queue::_thread_entry(void *param)
{
	timer_queue &tq = *reinterpret_cast<timer_queue *>(param);
	DWORD wait_time = INFINITE;

	while (true) {
		DWORD wait_result = ::WaitForSingleObject(tq.notify_event_.handle(), wait_time);
		if (wait_result == WAIT_FAILED) {
			throw win32_exception(::GetLastError());
		}

		while (true) {
			shared_ptr<context_base> context;
			{
				lock_scope<thread_mutex> lock(tq.contexts_mutex_);
				if (tq.contexts_.empty()) {
					if (tq.exiting_) {
						return 0;
					} else {
						wait_time = INFINITE;
						break;
					}
				}
				uint64_t expire = tq.contexts_.front()->expire();
				uint64_t now = util::get_tick_count();
				if (expire <= now) {
					pop_heap(tq.contexts_.begin(), tq.contexts_.end(), context_pred());
					context = move(tq.contexts_.back());
					tq.contexts_.pop_back();
				} else {
					const uint32_t timer_queue_jitter_ms = 10;
					wait_time = static_cast<DWORD>((expire - now) + timer_queue_jitter_ms);
				}

				if (context) {
					try {
						context->invoke();
					} catch (...) {
						assert(!"uncaught exception");
					}
				} else {
					break;
				}
			}
		}
	}
}

}
