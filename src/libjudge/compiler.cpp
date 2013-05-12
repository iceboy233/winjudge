#include <string>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <iterator>
#include <istream>
#include <fstream>
#include <sstream>
#include <winstl/filesystem/path.hpp>
#include <winstl/filesystem/pipe.hpp>
#include <judge.h>
#include <judgefs.h>
#include "compiler.hpp"
#include "pool.hpp"
#include "temp_dir.hpp"
#include "fs_filebuf.hpp"
#include "os_filebuf.hpp"
#include "bunny.hpp"
#include "exception.hpp"
#include "util.hpp"

using namespace std;
using winstl::path_a;

namespace judge {

compiler::compiler(
	const string &executable_path,
	const string &command_line,
	const string &source_filename,
	const string &target_filename,
	judge_limit &limit,
	const string &target_executable_path,
	const string &target_command_line)
	: executable_path_(executable_path)
	, command_line_(command_line)
	, source_filename_(source_filename)
	, target_filename_(target_filename)
	, limit_(limit)
	, target_executable_path_(target_executable_path)
	, target_command_line_(target_command_line)
{}

shared_ptr<temp_dir> compiler::_extract_source(
	pool &pool, judgefs *fs, const string &path)
{
	fs_filebuf fsbuf(fs, path);
	istream in(&fsbuf);

	shared_ptr<temp_dir> dir = pool.create_temp_dir("compile_");
	path_a out_path(dir->path());
	out_path.push(source_filename_.c_str());
	ofstream out(out_path.c_str(), ios::binary);

	const size_t buffer_size = 4096;
	bool result = util::stream_copy<buffer_size>(in, out);
	if (!result) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}
	return dir;
}

shared_ptr<compiler::result> compiler::compile(
	pool &pool, judgefs *source_fs, const string &source_path)
{
	shared_ptr<compiler::result> result(new compiler::result);
	result->compiler = shared_from_this();
	result->dir = _extract_source(pool, source_fs, source_path);

	if (!executable_path_.empty() || !command_line_.empty()) {
		vector<shared_ptr<env> > envs;
		pool.take_env(back_inserter(envs), 1);

		winstl::pipe pipe(0, false);
		judge::bunny bunny(*envs.front(), true, executable_path_, command_line_, result->dir->path(),
			NULL, pipe.write_handle(), pipe.write_handle(), limit_);
		pipe.close_write();

		// copy pipe stream
		os_filebuf pipe_read(pipe.read_handle(), false);
		istream in(&pipe_read);
		ostringstream out;
		bunny.start();
		const size_t buffer_size = 4096;
		if (limit_.stderr_output_limit) {
			util::stream_copy_n<buffer_size>(in, out, limit_.stderr_output_limit);
		} else {
			util::stream_copy<buffer_size>(in, out);
		}

		result->std_output = out.str();
		static_cast<judge::bunny::result &>(*result) = bunny.wait();
	} else {
		result->flag = JUDGE_ACCEPTED;
		result->exit_code = 0;
		result->time_usage_ms = 0;
		result->memory_usage_kb = 0;
		result->runtime_error = 0;
	}

	return result;
}

const string &compiler::executable_path()
{
	return executable_path_;
}

const string &compiler::command_line()
{
	return command_line_;
}

const string &compiler::source_filename()
{
	return source_filename_;
}

const string &compiler::target_filename()
{
	return target_filename_;
}

const judge_limit &compiler::limit()
{
	return limit_;
}

const string &compiler::target_executable_path()
{
	return target_executable_path_;
}

const string &compiler::target_command_line()
{
	return target_command_line_;
}

}
