#include <memory>
#include <string>
#include <cstring>
#include <cassert>
#include <utility>
#include <algorithm>
#include <winstl/synch/event.hpp>
#include <judgefs.h>
#include "test.hpp"
#include "testcase.hpp"
#include "pool.hpp"
#include "compiler.hpp"

using namespace std;
using winstl::event;

namespace judge {

struct test::context {
	explicit context(const std::shared_ptr<testcase> &testcase)
		: event(true, false)
		, testcase(testcase)
		, result()
	{}

	winstl::event event;
	std::shared_ptr<testcase> testcase;
	judge_result result;
};


test::test(const shared_ptr<pool> &pool,
	const shared_ptr<compiler> &compiler,
	judgefs *source_fs, const string &source_path)
	: pool_(pool)
	, compiler_(compiler)
	, source_fs_(source_fs)
	, source_path_(source_path)
	, phase_(JUDGE_PHASE_NO_MORE)
{
	memset(&summary_result_, 0, sizeof(summary_result_));
	judgefs_add_ref(source_fs_);
}

test::~test()
{
	judgefs_release(source_fs_);
}

void test::add(const shared_ptr<testcase> &testcase)
{
	contexts_.push_back(make_shared<context>(testcase));
}

void test::step()
{
	switch (phase_) {
	case JUDGE_PHASE_NO_MORE:
		phase_ = JUDGE_PHASE_COMPILE;
		index_ = 0;
		compiler_result_ = compiler_->compile(*pool_, source_fs_, source_path_);
		last_result_.reset(new judge_result);
		if (compiler_result_->flag == JUDGE_ACCEPTED) {
			if (compiler_result_->exit_code == 0) {
				last_result_->flag = JUDGE_ACCEPTED;
			} else {
				last_result_->flag = JUDGE_COMPILE_ERROR;
			}
		} else {
			last_result_->flag = compiler_result_->flag;
		}
		last_result_->time_usage_ms = compiler_result_->time_usage_ms;
		last_result_->memory_usage_kb = compiler_result_->memory_usage_kb;
		last_result_->runtime_error = compiler_result_->runtime_error;
		last_result_->judge_output = compiler_result_->std_output.c_str();
		last_result_->user_output = nullptr;
		last_context_.reset();
		return;

	case JUDGE_PHASE_COMPILE:
		phase_ = JUDGE_PHASE_TESTCASE;
		index_ = 0;
		break;

	case JUDGE_PHASE_TESTCASE:
		++index_;
		break;

	default:
		phase_ = JUDGE_PHASE_NO_MORE;
		index_ = 0;
		last_result_.reset();
		last_context_.reset();
		return;
	}

	assert(phase_ == JUDGE_PHASE_TESTCASE);
	assert(compiler_result_);

	if (compiler_result_->flag != JUDGE_ACCEPTED || compiler_result_->exit_code != 0) {
		summary_result_.flag = max(summary_result_.flag, JUDGE_COMPILE_ERROR);
	} else if (!contexts_.empty()) {
		if (index_ == 0) {
			for_each(contexts_.begin(), contexts_.end(),
				[&](shared_ptr<context> ctx)->void
			{
				vector<shared_ptr<env> > envs;
				pool_->take_env(back_inserter(envs), 1);
				shared_ptr<env> env = move(envs.front());
				shared_ptr<compiler::result> cr = compiler_result_;
				pool_->thread_pool().queue([env, ctx, cr]()->void {
					ctx->result = ctx->testcase->run(*env, *cr);
					ctx->event.set();
				});
			});
		}

		last_context_ = contexts_.front();
		contexts_.pop_front();
		if (::WaitForSingleObject(last_context_->event.get(), INFINITE) == WAIT_FAILED) {
			throw win32_exception(::GetLastError());
		}
		last_result_.reset(new judge_result(last_context_->result));
		summary_result_.flag = max(summary_result_.flag, last_result_->flag);
		summary_result_.time_usage_ms += last_result_->time_usage_ms;
		summary_result_.memory_usage_kb = max(summary_result_.memory_usage_kb, last_result_->memory_usage_kb);
		return;
	}

	phase_ = JUDGE_PHASE_SUMMARY;
	index_ = 0;
	last_result_.reset(new judge_result(summary_result_));
	last_context_.reset();
}

uint32_t test::phase()
{
	return phase_;
}

uint32_t test::index()
{
	return index_;
}

const judge_result &test::last_result()
{
	return *last_result_;
}

}
