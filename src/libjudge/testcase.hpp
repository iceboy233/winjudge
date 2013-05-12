#ifndef _TESTCASE_HPP
#define _TESTCASE_HPP

#include <memory>
#include <cstdint>
#include <string>
#include <judge.h>
#include <judgefs.h>
#include "compiler.hpp"

namespace judge {

class env;
class temp_dir;

class testcase {
public:
	testcase() {}
	virtual ~testcase() {}
	virtual judge_result run(env &env, compiler::result &cr) = 0;

protected:
	std::shared_ptr<temp_dir> _prepare_dir(judge::pool &pool, const compiler::result &cr);

private:
	// non-copyable
	testcase(const testcase &);
	testcase &operator=(const testcase &);
};

class testcase_impl : public testcase {
public:
	testcase_impl(judgefs *data_fs, const std::string &input_path,
		const std::string &output_path, judge_limit &limit);
	~testcase_impl();
	/* override */ judge_result run(env &env, compiler::result &cr);

private:
	struct context;

private:
	judgefs *data_fs_;
	std::string input_path_;
	std::string output_path_;
	judge_limit limit_;
	std::string judge_output_;
	std::string user_output_;
};

class testcase_spj_impl : public testcase {
public:
	testcase_spj_impl(judgefs *data_fs, const std::string &spj_prefix,
		const std::string &spj_path_rel, const std::string &spj_param,
		judge_limit &limit, judge_limit &spj_limit);
	/* override */ judge_result run(env &env, compiler::result &cr);

private:
	struct context;

private:
	judgefs *data_fs_;
	std::string spj_prefix_;
	std::string spj_path_rel_;
	std::string spj_param_;
	judge_limit limit_;
	judge_limit spj_limit_;
	std::shared_ptr<context> context_;
};

}

#endif
