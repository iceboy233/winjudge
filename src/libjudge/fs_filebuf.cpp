#include <cstdint>
#include <judgefs.h>
#include "fs_filebuf.hpp"
#include "exception.hpp"

using namespace std;

namespace judge {

fs_filebuf::fs_filebuf(judgefs *fs, judgefs_file *file, bool own_file)
	: fs_(fs), file_(file), own_file_(own_file)
{}

fs_filebuf::fs_filebuf(judgefs *fs, const std::string &path)
	: fs_(fs), file_(_open(fs, path)), own_file_(true)
{}

judgefs_file *fs_filebuf::_open(judgefs *fs, const std::string &path)
{
	judgefs_file *result;
	jstatus_t status = fs->vtable->open(fs, &result, path.c_str());
	if (!JSUCCESS(status)) {
		throw judge_exception(status);
	}
	return result;
}

fs_filebuf::~fs_filebuf()
{
	if (own_file_) {
		fs_->vtable->close(fs_, file_);
	}
}

int fs_filebuf::underflow()
{
	uint32_t actual;
	jstatus_t status = fs_->vtable->read(fs_, file_, gbuf_, buffer_size_, &actual);

	if (!JSUCCESS(status) || actual == 0)
		return EOF;
	setg(gbuf_, gbuf_, gbuf_ + actual);
	return static_cast<unsigned char>(*gbuf_);
}

}
