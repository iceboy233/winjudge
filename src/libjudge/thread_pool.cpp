#include <cstddef>
#include <cassert>
#include <winstl/synch/event.hpp>
#include <winstl/synch/atomic_types.h>
#include <winstl/synch/atomic_functions.h>
#include <windows.h>
#include <process.h>
#include "thread_pool.hpp"
#include "exception.hpp"

using namespace std;
using winstl::atomic_increment;
using winstl::atomic_decrement;
using winstl::atomic_predecrement;

namespace judge {

thread_pool::thread_pool()
	: queue_handle_(_create_queue())
	, thread_count_(0)
	, thread_exit_(true, false)
	, waiting_count_(0)
{
	_create_thread();
}

thread_pool::~thread_pool()
{
	::CloseHandle(queue_handle_);
}

void thread_pool::rundown()
{
	::PostQueuedCompletionStatus(queue_handle_, 0, 0, NULL);
	::WaitForSingleObject(thread_exit_.handle(), INFINITE);
}

HANDLE thread_pool::_create_queue()
{
	HANDLE result = reinterpret_cast<HANDLE>(
		::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0));
	if (!result) {
		throw win32_exception(::GetLastError());
	}
	return result;
}

void thread_pool::_create_thread()
{
	HANDLE result = reinterpret_cast<HANDLE>(
		::_beginthreadex(nullptr, 0, _thread_entry, this, 0, nullptr));
	if (!result) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}
	::CloseHandle(result);
	atomic_increment(&thread_count_);
}

unsigned int __stdcall thread_pool::_thread_entry(void *param)
{
	thread_pool &tp = *reinterpret_cast<thread_pool *>(param);
	DWORD size;
	ULONG_PTR key;
	LPOVERLAPPED overlapped;

	while (true) {
		atomic_increment(&tp.waiting_count_);
		BOOL success = ::GetQueuedCompletionStatus(tp.queue_handle_, &size, &key, &overlapped, INFINITE);
		int wc = atomic_predecrement(&tp.waiting_count_);
		if (!success) {
			if (atomic_predecrement(&tp.thread_count_) == 0) {
				tp.thread_exit_.set();
			}
			return 1;
		}

		if (!overlapped) {
			BOOL result = ::PostQueuedCompletionStatus(tp.queue_handle_, 0, 0, NULL);
			if (!result) {
				throw win32_exception(::GetLastError());
			}
			if (atomic_predecrement(&tp.thread_count_) == 0) {
				tp.thread_exit_.set();
			}
			return 0;
		}

		unique_ptr<context_base> context(reinterpret_cast<context_base *>(overlapped));
		if (wc <= 1) {
			try {
				tp._create_thread();
			} catch (...) {
			}
		}

		try {
			context->invoke();
		} catch (...) {
			assert(!"uncaught exception");
		}
	}
}

}
