#ifndef _TIMER_QUEUE_HPP
#define _TIMER_QUEUE_HPP

#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <cassert>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <winstl/synch/event.hpp>
#include <windows.h>

namespace judge {

class timer_queue {
public:
	timer_queue();
	~timer_queue();
	void rundown();

	template <typename T>
	void queue(T callback, unsigned delay_ms);

private:
	// non-copyable
	timer_queue(const timer_queue &);
	timer_queue &operator=(const timer_queue &);

private:
	class context_base {
	public:
		context_base(unsigned delay_ms);
		virtual ~context_base() {}
		std::uint64_t expire();
		virtual void invoke() = 0;
	private:
		std::uint64_t expire_;
	};

	template <typename T>
	class context : public context_base {
	public:
		context(T callback, unsigned delay_ms);
		/* override */ void invoke();

	private:
		T callback_;
	};

	struct context_pred : public std::binary_function<
		std::shared_ptr<context_base>,
		std::shared_ptr<context_base>,
		bool> {
		bool operator()(const std::shared_ptr<context_base> &a,
		                const std::shared_ptr<context_base> &b) {
			return a->expire() > b->expire();
		}
	};

private:
	HANDLE _create_thread();
	static unsigned int __stdcall _thread_entry(void *param);

private:
	std::vector<std::shared_ptr<context_base> > contexts_;
	winstl::thread_mutex contexts_mutex_;
	winstl::event notify_event_;
	volatile bool exiting_;
	HANDLE thread_handle_;
};

template <typename T>
void timer_queue::queue(T callback, unsigned delay_ms)
{
	assert(!exiting_);
	{
		stlsoft::lock_scope<winstl::thread_mutex> lock(contexts_mutex_);
		contexts_.push_back(std::shared_ptr<context_base>(new context<T>(callback, delay_ms)));
		std::push_heap(contexts_.begin(), contexts_.end(), context_pred());
	}
	notify_event_.set();
}

template <typename T>
timer_queue::context<T>::context(T callback, unsigned delay_ms)
	: context_base(delay_ms), callback_(callback)
{}

template <typename T>
void timer_queue::context<T>::invoke()
{
	callback_();
}

}

#endif
