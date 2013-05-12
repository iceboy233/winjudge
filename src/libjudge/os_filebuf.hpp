#ifndef _OS_FILEBUF_HPP
#define _OS_FILEBUF_HPP

#include <streambuf>
#include <cstddef>
#include <windows.h>

namespace judge {

class os_filebuf : public std::streambuf {
public:
	explicit os_filebuf(HANDLE handle, bool own_handle = true);
	virtual ~os_filebuf();
	HANDLE handle();

private:
	// non-copyable
	os_filebuf(const os_filebuf &);
	os_filebuf &operator =(const os_filebuf &);

protected:
	/* override */ int overflow(int c = EOF);
	/* override */ int sync();
	/* override */ int underflow();

private:
	HANDLE handle_;
	bool own_handle_;

	static const std::size_t buffer_size_ = 4096;
	char pbuf_[buffer_size_];
	char gbuf_[buffer_size_];
};

}

#endif
