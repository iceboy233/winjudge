#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <vector>
#include <winstl/synch/event.hpp>
#include <winstl/synch/atomic_types.h>
#include <windows.h>
#include "exception.hpp"

namespace judge {

class thread_pool {
public:
	thread_pool();
	~thread_pool();
	void rundown();

	template <typename T>
	void queue(T callback);

private:
	// non-copyable
	thread_pool(const thread_pool &);
	thread_pool &operator=(const thread_pool &);

private:
	HANDLE _create_queue();
	void _create_thread();
	static unsigned int __stdcall _thread_entry(void *param);

private:
	class context_base {
	public:
		virtual ~context_base() {}
		virtual void invoke() = 0;
	};

	template <typename T>
	class context : public context_base {
	public:
		context(T callback);
		/* override */ void invoke();

	private:
		T callback_;
	};

private:
	HANDLE queue_handle_;
	volatile winstl::atomic_int_t thread_count_;
	winstl::event thread_exit_;
	volatile winstl::atomic_int_t waiting_count_;
};

template <typename T>
void thread_pool::queue(T callback)
{
	context_base *p = new context<T>(std::move(callback));
	BOOL result = ::PostQueuedCompletionStatus(queue_handle_, 0, 0, reinterpret_cast<LPOVERLAPPED>(p));
	if (!result) {
		DWORD error_code = ::GetLastError();
		delete p;
		throw win32_exception(error_code);
	}
}

template <typename T>
thread_pool::context<T>::context(T callback)
	: callback_(callback)
{}

template <typename T>
void thread_pool::context<T>::invoke()
{
	callback_();
}

}

#endif
