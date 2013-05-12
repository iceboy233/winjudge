#ifndef _ENV_HPP
#define _ENV_HPP

#include <string>
#include <memory>
#include <utility>
#include <cstddef>
#include <windows.h>
#include <winstl/filesystem/path.hpp>
#include "process.hpp"
#include "thread_suspension.hpp"
#include "logon_session.hpp"
#include "desktop.hpp"
#include "window_station.hpp"

namespace judge {

class pool;

class env {
public:
	env(const std::shared_ptr<judge::pool> &pool);
	virtual ~env() {}

	virtual std::pair<std::shared_ptr<process>,
		std::shared_ptr<thread_suspension> > create_process(
		const std::string &executable_path,
		const std::string &command_line,
		const winstl::path_a &current_dir,
		HANDLE stdin_handle,
		HANDLE stdout_handle,
		HANDLE stderr_handle);

	std::pair<std::shared_ptr<process>,
		std::shared_ptr<thread_suspension> > create_process_trusted(
		const std::string &executable_path,
		const std::string &command_line,
		const winstl::path_a &current_dir,
		HANDLE stdin_handle,
		HANDLE stdout_handle,
		HANDLE stderr_handle);

	virtual void grant_access(const winstl::path_a &path) {};
	pool &pool();

private:
	// non-copyable
	env(const env &);
	env &operator=(const env &);

private:
	std::shared_ptr<judge::pool> pool_;
};

class restricted_env : public env {
public:
	restricted_env(const std::shared_ptr<judge::pool> &pool,
		const std::string &username, const std::string &password);

	/* override */ std::pair<std::shared_ptr<process>,
		std::shared_ptr<thread_suspension> > create_process(
		const std::string &executable_path,
		const std::string &command_line,
		const winstl::path_a &current_dir,
		HANDLE stdin_handle,
		HANDLE stdout_handle,
		HANDLE stderr_handle);

	/* override */ void grant_access(const winstl::path_a &path);

private:
	std::string username_;
	logon_session session_;
	window_station window_station_;
	static const std::size_t desktop_name_length = 16;
	desktop desktop_;
};

}

#endif
