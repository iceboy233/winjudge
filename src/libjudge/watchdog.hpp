#ifndef _WATCHDOG_HPP
#define _WATCHDOG_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <cassert>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/event.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <windows.h>
#include "process.hpp"
#include "job_object.hpp"

namespace judge {

class watchdog {
private:
	class context_base {
	public:
		context_base(const std::shared_ptr<judge::process> &ps,
			const std::shared_ptr<judge::job_object> &job,
			std::uint32_t time_limit_ms);
		virtual ~context_base() {}
		virtual void invoke() = 0;
		const std::shared_ptr<judge::process> process();
		const std::shared_ptr<judge::job_object> job_object();
		std::uint32_t time_limit_ms();
	private:
		std::shared_ptr<judge::process> process_;
		std::shared_ptr<judge::job_object> job_object_;
		std::uint32_t time_limit_ms_;
	};

	template <typename T>
	class context : public context_base {
	public:
		context(const std::shared_ptr<judge::process> &ps,
			const std::shared_ptr<judge::job_object> &job,
			std::uint32_t time_limit_ms, T completion);
		/* override */ void invoke();
	private:
		T completion_;
	};

public:
	watchdog();
	~watchdog();
	void rundown();

	template <typename T>
	void add(const std::shared_ptr<process> &ps,
		const std::shared_ptr<job_object> &job,
		std::uint32_t time_limit_ms, T completion);

private:
	// non-copyable
	watchdog(const watchdog &);
	watchdog &operator=(const watchdog &);

private:
	HANDLE _create_thread();
	static unsigned int __stdcall _thread_entry(void *param);

private:
	winstl::event meta_event_;
	volatile bool exiting_;
	winstl::thread_mutex contexts_mutex_;
	std::vector<std::shared_ptr<context_base> > contexts_;
	HANDLE thread_handle_;
};

template <typename T>
watchdog::context<T>::context(
	const std::shared_ptr<judge::process> &ps,
	const std::shared_ptr<judge::job_object> &job,
	std::uint32_t time_limit_ms, T completion)
	: context_base(ps, job, time_limit_ms)
	, completion_(completion)
{}

template <typename T>
void watchdog::context<T>::invoke()
{
	completion_();
}

template <typename T>
void watchdog::add(const std::shared_ptr<process> &ps,
	const std::shared_ptr<job_object> &job,
	std::uint32_t time_limit_ms, T completion)
{
	assert(!exiting_);
	std::shared_ptr<context_base> ctx(new context<T>(ps, job, time_limit_ms, completion));
	{
		stlsoft::lock_scope<winstl::thread_mutex> lock(contexts_mutex_);
		contexts_.insert(contexts_.end(), std::move(ctx));
	}
	meta_event_.set();
}

}

#endif
