#ifndef _COMPILER_HPP
#define _COMPILER_HPP

#include <memory>
#include <string>
#include <cstdint>
#include <judge.h>
#include <judgefs.h>
#include "bunny.hpp"

namespace judge {

class temp_dir;
class pool;

class compiler : public std::enable_shared_from_this<compiler> {
public:
	struct result : public bunny::result {
		std::shared_ptr<compiler> compiler;
		std::shared_ptr<temp_dir> dir;
		std::string std_output;
	};

public:
	compiler(
		// Compiler settings
		const std::string &executable_path,
		const std::string &command_line,
		const std::string &source_filename,
		const std::string &target_filename,
		judge_limit &limit,

		// Target settings
		const std::string &target_executable_path,
		const std::string &target_command_line);

	std::shared_ptr<compiler::result> compiler::compile(
		pool &pool, judgefs *source_fs, const std::string &source_path);

private:
	// non-copyable
	compiler(const compiler &);
	compiler &operator=(const compiler &);

public:
	const std::string &executable_path();
	const std::string &command_line();
	const std::string &source_filename();
	const std::string &target_filename();
	const judge_limit &limit();
	const std::string &target_executable_path();
	const std::string &target_command_line();

private:
	std::shared_ptr<temp_dir> _extract_source(
		pool &pool, judgefs *fs, const std::string &path);

private:
	std::string executable_path_;
	std::string command_line_;
	std::string source_filename_;
	std::string target_filename_;
	judge_limit limit_;
	std::string target_executable_path_;
	std::string target_command_line_;
};

}

#endif
