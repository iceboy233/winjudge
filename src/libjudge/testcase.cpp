#include <memory>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <istream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <winstl/filesystem/path.hpp>
#include <winstl/filesystem/pipe.hpp>
#include <winstl/synch/event.hpp>
#include <winstl/synch/wait_functions.hpp>
#include <windows.h>
#include <judge.h>
#include "onullstream.hpp"
#include "env.hpp"
#include "pool.hpp"
#include "temp_dir.hpp"
#include "testcase.hpp"
#include "fs_filebuf.hpp"
#include "os_filebuf.hpp"
#include "exception.hpp"
#include "util.hpp"

using namespace std;
using namespace judge;
using winstl::path_a;

namespace {

pair<char, char> read_char(istream &in, bool first)
{
	char whitespace = '\0';
	char data = '\0';
	char ch;

	while ((ch = in.get()) != EOF) {
		if (ch == '\r' || ch == '\n') {
			if (!first) {
				whitespace = '\n';
			}
		} else if (ch == ' ' || ch == '\t') {
			if (!first && whitespace != '\n') {
				whitespace = ' ';
			}
		} else {
			data = ch;
			break;
		}
	}

	return make_pair(whitespace, data);
}

void append_char(string &str, pair<char, char> ch, size_t max_length)
{
	if (ch.first == '\n')
		str.clear();
	if (ch.first == ' ' && str.size() < max_length)
		str.push_back(' ');
	if (ch.second != '\0' && str.size() < max_length)
		str.push_back(ch.second);
}

pair<bool, string> compare_stream(istream &model, istream &user)
{
	bool first = true;
	string model_line, user_line;
	while (true) {
		pair<char, char> model_char = read_char(model, first);
		pair<char, char> user_char = read_char(user, first);
		first = false;
		if (model_char.second == '\0' && user_char.second == '\0') {
			return make_pair(true, string());
		}
		if (model_char != user_char) {
			while (model_char.first != '\n' && model_char.second != '\0') {
				append_char(model_line, model_char, 4096);
				model_char = read_char(model, first);
			}
			while (user_char.first != '\n' && user_char.second != '\0') {
				append_char(user_line, user_char, 4096);
				user_char = read_char(user, first);
			}
			return make_pair(false, "model " + model_line + "\nuser " + user_line);
		}
		const size_t max_length = 4096;
		append_char(model_line, model_char, max_length);
		append_char(user_line, user_char, max_length);
	}
}

void file_copy(const winstl::path_a &in_path, const winstl::path_a &out_path)
{
	ifstream in(in_path.c_str(), ios::binary);
	if (!in.is_open()) {
		throw judge_exception(JSTATUS_NOT_FOUND);
	}

	ofstream out(out_path.c_str(), ios::binary);
	const size_t buffer_size = 4096;
	if (!util::stream_copy<buffer_size>(in, out)) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}
}

}

namespace judge {

shared_ptr<temp_dir> testcase::_prepare_dir(judge::pool &pool, const compiler::result &cr)
{
	shared_ptr<temp_dir> dir = pool.create_temp_dir("test_");
	path_a in_path(cr.dir->path());
	in_path.push(cr.compiler->target_filename().c_str());
	path_a out_path(dir->path());
	out_path.push(cr.compiler->target_filename().c_str());
	file_copy(in_path, out_path);
	return dir;
}

struct testcase_impl::context {
	context(testcase_impl &testcase)
		: stderr_output_limit(testcase.limit_.stderr_output_limit)
		, stdin_pipe(0, false)
		, stdout_pipe(0, false)
		, stderr_pipe(0, false)
		, stdin_event(true, false)
		, stderr_event(true, false)
		, stdin_buf(testcase.data_fs_, testcase.input_path_)
		, stdout_buf(testcase.data_fs_, testcase.output_path_)
	{}

	uint32_t stderr_output_limit;
	winstl::pipe stdin_pipe;
	winstl::pipe stdout_pipe;
	winstl::pipe stderr_pipe;
	winstl::event stdin_event;
	winstl::event stderr_event;
	fs_filebuf stdin_buf;
	fs_filebuf stdout_buf;
	ostringstream stderr_stream;
};

testcase_impl::testcase_impl(judgefs *data_fs, const string &input_path,
	const string &output_path, judge_limit &limit)
	: data_fs_(data_fs)
	, input_path_(input_path)
	, output_path_(output_path)
	, limit_(limit)
{
	judgefs_add_ref(data_fs_);
}

testcase_impl::~testcase_impl()
{
	judgefs_release(data_fs_);
}

judge_result testcase_impl::run(env &env, compiler::result &cr)
{
	judge_result result = {0};
	shared_ptr<temp_dir> dir = _prepare_dir(env.pool(), cr);
	env.grant_access(dir->path());

	path_a executable_path;
	if (cr.compiler->target_executable_path().empty()) {
		executable_path = dir->path();
		executable_path.push(cr.compiler->target_filename().c_str());
	} else {
		executable_path = cr.compiler->target_executable_path();
	}

	shared_ptr<testcase_impl::context> context(new testcase_impl::context(*this));
	judge::bunny bunny(env, false, executable_path.c_str(), cr.compiler->target_command_line(), dir->path(),
		context->stdin_pipe.read_handle(), context->stdout_pipe.write_handle(), context->stderr_pipe.write_handle(), limit_);
	context->stdin_pipe.close_read();
	context->stdout_pipe.close_write();
	context->stderr_pipe.close_write();

	// stdin thread
	env.pool().thread_pool().queue([context]()->void {
		try {
			istream in(&context->stdin_buf);
			os_filebuf out_buf(context->stdin_pipe.write_handle(), false);
			ostream out(&out_buf);
			const size_t buffer_size = 4096;
			util::stream_copy<buffer_size>(in, out);
		} catch (...) {
		}
		context->stdin_pipe.close_write();
		context->stdin_event.set();
	});

	// stderr thread
	env.pool().thread_pool().queue([context]()->void {
		try {
			os_filebuf in_buf(context->stderr_pipe.read_handle(), false);
			istream in(&in_buf);
			const size_t buffer_size = 4096;
			util::stream_copy_n<buffer_size>(in, context->stderr_stream, context->stderr_output_limit);
		} catch (...) {
		}
		context->stderr_pipe.close_read();
		context->stderr_event.set();
	});

	bunny.start();

	// judge
	{
		istream model_in(&context->stdout_buf);
		os_filebuf user_buf(context->stdout_pipe.read_handle(), false);
		istream user_in(&user_buf);
		pair<bool, string> compare_result = compare_stream(model_in, user_in);
		if (compare_result.first) {
			result.flag = max(result.flag, JUDGE_ACCEPTED);
		} else {
			result.flag = max(result.flag, JUDGE_WRONG_ANSWER);
		}
		judge_output_ = move(compare_result.second);

		// read all user output
		const size_t buffer_size = 4096;
		util::stream_copy<buffer_size>(user_in, onullstream());
	}

	bunny::result bunny_result = bunny.wait();
	DWORD wait_result = winstl::wait_for_multiple_objects(context->stdin_event, context->stderr_event, true, INFINITE);
	if (wait_result == WAIT_FAILED) {
		throw win32_exception(::GetLastError());
	}

	result.flag = max(result.flag, bunny_result.flag);
	result.time_usage_ms = bunny_result.time_usage_ms;
	result.memory_usage_kb = bunny_result.memory_usage_kb;
	result.runtime_error = bunny_result.runtime_error;
	result.judge_output = judge_output_.c_str();
	user_output_ = context->stderr_stream.str();
	result.user_output = user_output_.c_str();
	return result;
}

testcase_spj_impl::testcase_spj_impl(judgefs *data_fs, const std::string &spj_prefix,
	const std::string &spj_path_rel, const std::string &spj_param,
	judge_limit &limit, judge_limit &spj_limit)
	: data_fs_(data_fs)
	, spj_prefix_(spj_prefix)
	, spj_path_rel_(spj_path_rel)
	, spj_param_(spj_param)
	, limit_(limit)
	, spj_limit_(limit)
{}

judge_result testcase_spj_impl::run(env &env, compiler::result &cr)
{
	// TODO: not implemented
	throw judge_exception(JSTATUS_NOT_IMPLEMENTED);
}

}
