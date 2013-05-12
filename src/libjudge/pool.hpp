#ifndef _POOL_HPP
#define _POOL_HPP

#include <string>
#include <memory>
#include <cstddef>
#include <cassert>
#include <vector>
#include <stack>
#include <queue>
#include <algorithm>
#include <winstl/synch/thread_mutex.hpp>
#include <winstl/synch/event.hpp>
#include <winstl/filesystem/path.hpp>
#include <stlsoft/synch/lock_scope.hpp>
#include <windows.h>
extern "C" {
	#include <ntndk.h>
}
#include "env.hpp"
#include "exception.hpp"
#include "process_monitor.hpp"
#include "watchdog.hpp"
#include "timer_queue.hpp"
#include "thread_pool.hpp"

namespace judge {

struct pool_req;
class temp_dir;

class pool : public std::enable_shared_from_this<pool> {
public:
	pool() {}
	void set_temp_dir(const std::shared_ptr<temp_dir> &temp_dir);
	void add_env(std::unique_ptr<env> &&env);

	template <typename OutIt>
	void take_env(OutIt dest, std::size_t count);

	std::shared_ptr<temp_dir> create_temp_dir(const std::string &prefix);
	judge::process_monitor &process_monitor();
	judge::watchdog &watchdog();
	judge::timer_queue &timer_queue();
	judge::thread_pool &thread_pool();

private:
	// non-copyable
	pool(const pool &);
	pool &operator=(const pool &);

private:
	void _satisfy_unlocked();

private:
	class untake_deleter {
	public:
		untake_deleter(const std::shared_ptr<pool> &p);
		void operator()(env *e);
	private:
		std::shared_ptr<pool> p;
	};

private:
	judge::process_monitor process_monitor_;
	judge::watchdog watchdog_;
	judge::timer_queue timer_queue_;
	judge::thread_pool thread_pool_;
	std::shared_ptr<temp_dir> temp_dir_;
	std::size_t env_count_;
	std::stack<std::unique_ptr<env> > env_stack_;
	std::queue<pool_req *> req_queue_;
	winstl::thread_mutex req_queue_mutex_;


};

struct pool_req {
	explicit pool_req(std::size_t count);

	std::size_t count;
	std::vector<std::shared_ptr<env> > envs;
	winstl::event event;
};

template <typename OutIt>
void pool::take_env(OutIt dest, std::size_t count)
{
	if (count > env_count_) {
		throw judge_exception(JSTATUS_CONCURRENCY_LIMIT_EXCEEDED);
	}

	pool_req req(count);
	{
		stlsoft::lock_scope<winstl::thread_mutex> lock(req_queue_mutex_);
		req_queue_.push(&req);
		_satisfy_unlocked();
	}

	if (::WaitForSingleObject(req.event.get(), INFINITE) == WAIT_FAILED) {
		throw win32_exception(::GetLastError());
	}

	std::copy(req.envs.begin(), req.envs.end(), dest);
}

}

#endif
