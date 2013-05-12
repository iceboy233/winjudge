#ifndef _TEMP_DIR_HPP
#define _TEMP_DIR_HPP

#include <memory>
#include <string>
#include <winstl/filesystem/path.hpp>
#include "pool.hpp"

namespace judge {

class temp_dir : public std::enable_shared_from_this<temp_dir> {
public:
	temp_dir(const std::shared_ptr<pool> &pool, const winstl::path_a &path,
		bool delete_on_close = false);
	temp_dir(const std::shared_ptr<pool> &pool, const winstl::path_a &path,
		const std::shared_ptr<temp_dir> &parent, bool delete_on_close = false);
	~temp_dir();
	
	void create_if_not_exist();
	const winstl::path_a &path();
	const std::shared_ptr<temp_dir> &parent();
	std::shared_ptr<temp_dir> make_sub(const std::string &prefix, bool delete_on_close = true);

private:
	// non-copyable
	temp_dir(const temp_dir &);
	temp_dir &operator =(const temp_dir &);

private:
	std::shared_ptr<pool> pool_;
	std::shared_ptr<winstl::path_a> path_;
	std::shared_ptr<temp_dir> parent_;
	bool delete_on_close_;
};

}

#endif
