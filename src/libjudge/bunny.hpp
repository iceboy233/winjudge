#ifndef _BUNNY_HPP
#define _BUNNY_HPP

#include <memory>
#include <string>
#include <winstl/filesystem/path.hpp>
#include <windows.h>
#include <judge.h>
#include "env.hpp"
#include "job_object.hpp"

namespace judge {

class process;
class thread_suspension;

// a bunny is a running process with job object, watchdog and process monitor attached

class bunny {
public:
	struct result {
		judge_flag_t flag;
		std::int32_t exit_code;
		std::uint32_t time_usage_ms;
		std::uint32_t memory_usage_kb;
		runtime_error_t runtime_error;
	};

private:
	struct context;

public:
	bunny(env &env, bool trusted,
		const std::string &executable_path, const std::string &command_line, const winstl::path_a &current_dir,
		HANDLE stdin_handle, HANDLE stdout_handle, HANDLE stderr_handle, const judge_limit &limit);

	void start();
	bunny::result wait();

private:
	// non-copyable
	bunny(const bunny &);
	bunny &operator=(const bunny &);
	
private:
	void _tune_job_object();
	uint64_t _get_section_size(const std::string &executable_path);

private:
	std::shared_ptr<process> process_;
	std::shared_ptr<thread_suspension> thread_suspension_;
	std::shared_ptr<job_object> job_;
	std::shared_ptr<context> context_;
	judge_limit limit_;
	std::uint64_t section_size_;
};

}

#endif
