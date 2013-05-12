#include <string>
#include <memory>
#include <cstddef>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <winstl/filesystem/path.hpp>
#include "pool.hpp"
#include "env.hpp"
#include "temp_dir.hpp"

using namespace std;
using stlsoft::lock_scope;
using winstl::thread_mutex;
using winstl::path_a;

namespace judge {

void pool::set_temp_dir(const std::shared_ptr<temp_dir> &temp_dir)
{
	temp_dir_ = temp_dir;
}

void pool::add_env(unique_ptr<env> &&env)
{
	lock_scope<thread_mutex> lock(req_queue_mutex_);
	env_stack_.push(move(env));
	++env_count_;
}

shared_ptr<temp_dir> pool::create_temp_dir(const string &prefix)
{
	return temp_dir_->make_sub(prefix);
}

judge::process_monitor &pool::process_monitor()
{
	return process_monitor_;
}

judge::watchdog &pool::watchdog()
{
	return watchdog_;
}

judge::timer_queue &pool::timer_queue()
{
	return timer_queue_;
}

judge::thread_pool &pool::thread_pool()
{
	return thread_pool_;
}

void pool::_satisfy_unlocked()
{
	while (!req_queue_.empty() &&
	       req_queue_.front()->count <= env_stack_.size()) {

		pool_req *req = req_queue_.front();
		while (req->count != 0) {
			shared_ptr<env> e(env_stack_.top().release(), untake_deleter(shared_from_this()));
			env_stack_.pop();
			req->envs.push_back(e);
			--req->count;
		}
		req_queue_.pop();
		req->event.set();
	}
}

pool::untake_deleter::untake_deleter(const shared_ptr<pool> &p)
	: p(p)
{}

void pool::untake_deleter::operator()(env *e)
{
	lock_scope<thread_mutex> lock(p->req_queue_mutex_);
	p->env_stack_.push(unique_ptr<env>(e));
	p->_satisfy_unlocked();
}

pool_req::pool_req(size_t count)
	: count(count)
	, event(true, false)
{
	envs.reserve(count);
}

}
