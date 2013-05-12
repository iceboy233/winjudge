#ifndef _FS_FILEBUF_HPP
#define _FS_FILEBUF_HPP

#include <streambuf>
#include <string>
#include <judgefs.h>

namespace judge {

class fs_filebuf : public std::streambuf {
public:
	fs_filebuf(judgefs *fs, judgefs_file *file, bool own_file = true);
	fs_filebuf(judgefs *fs, const std::string &path);
	virtual ~fs_filebuf();
	
private:
	judgefs_file *_open(judgefs *fs, const std::string &path);

private:
	// non-copyable
	fs_filebuf(const fs_filebuf &);
	fs_filebuf &operator =(const fs_filebuf &);

protected:
	/* override */ int underflow();

private:
	judgefs *fs_;
	judgefs_file *file_;
	bool own_file_;

	static const std::size_t buffer_size_ = 4096;
	char gbuf_[buffer_size_];
};

}

#endif
