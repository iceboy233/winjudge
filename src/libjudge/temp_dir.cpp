#include <string>
#include <memory>
#include <cstddef>
#include <winstl/filesystem/path.hpp>
#include <winstl/filesystem/directory_functions.hpp>
#include "temp_dir.hpp"
#include "exception.hpp"
#include "random_string.hpp"

using namespace std;
using namespace judge;
using winstl::path_a;
using winstl::create_directory_recurse;
using winstl::remove_directory_recurse;
using winstl::ws_int_t;
using winstl::ws_char_a_t;

namespace {

const unsigned initial_delay_ms = 1000;
const unsigned retry_limit = 10;

class path_deletion {
public:
	path_deletion(const shared_ptr<pool> pool, unsigned delay_ms, unsigned retry_limit, const shared_ptr<path_a> &path)
		: pool_(pool)
		, delay_ms_(delay_ms)
		, retry_limit_(retry_limit)
		, path_(path)
	{}

	void operator()()
	{
		bool success;
		try {
			success = remove_directory_recurse(path_->c_str(), path_deletion_callback, this);
		} catch (...) {
			success = false;
		}

		if (!success && retry_limit_ > 0) {
			pool_->timer_queue().queue(
				path_deletion(pool_, delay_ms_ * 2, retry_limit_ - 1, path_),
				delay_ms_);
		}
	}

private:
	static ws_int_t path_deletion_callback(void *param,
		ws_char_a_t const* subDir, WIN32_FIND_DATAA const* st, DWORD err)
	{
		return true;
	}

private:
	shared_ptr<pool> pool_;
	unsigned delay_ms_;
	unsigned retry_limit_;
	shared_ptr<path_a> path_;
};

}

namespace judge {


temp_dir::temp_dir(
	const std::shared_ptr<pool> &pool,
	const path_a &path,
	bool delete_on_close)
	: pool_(pool)
	, path_(new path_a(path))
	, delete_on_close_(delete_on_close)
{}

temp_dir::temp_dir(
	const std::shared_ptr<pool> &pool,
	const path_a &path,
	const shared_ptr<temp_dir> &parent,
	bool delete_on_close)
	: pool_(pool)
	, path_(new path_a(path))
	, parent_(parent)
	, delete_on_close_(delete_on_close)
{}

temp_dir::~temp_dir()
{
	if (delete_on_close_) {
		path_deletion(pool_, initial_delay_ms, retry_limit, path_)();
	}
}

void temp_dir::create_if_not_exist()
{
	if (!path().exists()) {
		if (!create_directory_recurse(path())) {
			throw judge_exception(JSTATUS_GENERIC_ERROR);
		}
	}
}

const path_a &temp_dir::path()
{
	return *path_;
}

const shared_ptr<temp_dir> &temp_dir::parent()
{
	return parent_;
}

shared_ptr<temp_dir> temp_dir::make_sub(const string &prefix, bool delete_on_close)
{
	create_if_not_exist();
	path_a new_path;

	do {
		new_path = path();
		string name;
		const size_t name_len = 16;
		new_path.push((prefix + random_string(name_len)).c_str());
	} while (new_path.exists());

	shared_ptr<temp_dir> result(new temp_dir(pool_, new_path, shared_from_this(), delete_on_close));
	result->create_if_not_exist();
	return result;
}

}
