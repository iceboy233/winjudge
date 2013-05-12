#ifndef _TEST_HPP
#define _TEST_HPP

#include <memory>
#include <string>
#include <deque>
#include <cstdint>
#include <utility>
#include <winstl/synch/event.hpp>
#include <judge.h>
#include <judgefs.h>
#include "compiler.hpp"

namespace judge {

class pool;
class testcase;

class test {
public:
	test(const std::shared_ptr<pool> &pool,
		const std::shared_ptr<compiler> &compiler,
		judgefs *source_fs, const std::string &source_path);
	~test();
	
	void add(const std::shared_ptr<testcase> &testcase);
	void step();
	std::uint32_t phase();
	std::uint32_t index();
	const judge_result &last_result();

private:
	// non-copyable
	test(const test &);
	test &operator=(const test &);

private:
	struct context;

private:
	std::shared_ptr<pool> pool_;
	std::shared_ptr<compiler> compiler_;
	judgefs *source_fs_;
	std::string source_path_;
	std::deque<std::shared_ptr<context> > contexts_;
	std::uint32_t phase_;
	std::uint32_t index_;
	std::shared_ptr<context> last_context_;
	std::shared_ptr<compiler::result> compiler_result_;
	judge_result last_result_;
	judge_result summary_result_;
};

}

#endif
