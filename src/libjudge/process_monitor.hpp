#ifndef _PROCESS_MONITOR_HPP
#define _PROCESS_MONITOR_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <windows.h>
#include "debug_object.hpp"
#include "process.hpp"
#include "job_object.hpp"
#include "util.hpp"

namespace judge {

class process_monitor {
public:
	struct result {
		result();

		std::int32_t exit_status;
		bool has_exception;
		EXCEPTION_RECORD exception;
	};

private:
	class context_base {
	public:
		context_base(const std::shared_ptr<judge::process> &ps,
			const std::shared_ptr<judge::job_object> &job);
		virtual ~context_base() {}
		virtual void invoke() = 0;
		void set_exit_status(std::int32_t exit_status);
		void set_exception(const EXCEPTION_RECORD &exception);
		judge::process &process();
		judge::job_object &job_object();
		void add_thread(uint32_t thread_id, util::safe_handle_t thread_handle);
		util::safe_handle_t get_thread(uint32_t thread_id);
		void set_breakpoint_at_entry(void *base);
		bool on_breakpoint(void *address);
	protected:
		result result_;
	private:
		std::shared_ptr<judge::process> ps_;
		std::shared_ptr<judge::job_object> job_;
		void *entry_point_;
		BYTE stolen_byte_;
		std::unordered_map<uint32_t, util::safe_handle_t> threads_;
	};

	template <typename T>
	class context : public context_base {
	public:
		context(const std::shared_ptr<judge::process> &ps,
			const std::shared_ptr<judge::job_object> &job, T completion);
		/* override */ void invoke();
	private:
		T completion_;
	};

	class debugger : public debug_object {
	public:
		debugger(process_monitor &pm);
	protected:
		/* override */ NTSTATUS on_create_process(PDBGUI_WAIT_STATE_CHANGE state_change);
		/* override */ NTSTATUS on_create_thread(PDBGUI_WAIT_STATE_CHANGE state_change);
		/* override */ NTSTATUS on_exit_process(PDBGUI_WAIT_STATE_CHANGE state_change);
		/* override */ NTSTATUS on_exception(PDBGUI_WAIT_STATE_CHANGE state_change);
	private:
		std::shared_ptr<context_base> _lookup_context(uint32_t process_id, bool erase = false);
	private:
		process_monitor &pm_;
	};

public:
	process_monitor();
	~process_monitor();
	void rundown();

	template <typename T>
	void add(const std::shared_ptr<judge::process> &ps,
		const std::shared_ptr<judge::job_object> &job, T completion);

private:
	// non-copyable
	process_monitor(const process_monitor &);
	process_monitor &operator=(const process_monitor &);

private:
	HANDLE _create_thread();
	static unsigned int __stdcall _thread_entry(void *param);

private:
	debugger debugger_;
	winstl::thread_mutex contexts_mutex_;
	std::unordered_map<std::uint32_t, std::shared_ptr<context_base> > contexts_;
	HANDLE thread_handle_;
};

template <typename T>
process_monitor::context<T>::context(
	const std::shared_ptr<judge::process> &ps,
	const std::shared_ptr<judge::job_object> &job, T completion)
	: context_base(ps, job), completion_(completion)
{}

template <typename T>
void process_monitor::context<T>::invoke()
{
	completion_(std::move(result_));
}

template <typename T>
void process_monitor::add(const std::shared_ptr<judge::process> &ps,
	const std::shared_ptr<judge::job_object> &job, T completion)
{
	stlsoft::lock_scope<winstl::thread_mutex> lock(contexts_mutex_);
	std::shared_ptr<context_base> context(new context<T>(ps, job, completion));
	auto iter = contexts_.insert(std::make_pair(ps->id(), std::move(context)));
	try {
		debugger_.attach(ps->handle());
	} catch (...) {
		contexts_.erase(iter.first);
		throw;
	}
}

}

#endif
