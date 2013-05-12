#include <streambuf>
#include <cstddef>
#include <windows.h>
#include "os_filebuf.hpp"

using namespace std;

namespace judge {

os_filebuf::os_filebuf(HANDLE handle, bool own_handle)
	: handle_(handle), own_handle_(own_handle)
{
	setp(pbuf_, pbuf_ + buffer_size_ - 1);
}

os_filebuf::~os_filebuf()
{
	sync();
	if (own_handle_)
		::CloseHandle(handle());
}

HANDLE os_filebuf::handle()
{
	return handle_;
}

int os_filebuf::overflow(int c)
{
	char *begin = pbase();
	char *end = pptr();

	if (c != EOF) {
		*end++ = c;
	}

	size_t size = end - begin;
	DWORD actual;
	BOOL result = ::WriteFile(handle(), begin, static_cast<DWORD>(size), &actual, NULL);
	setp(pbuf_, pbuf_, pbuf_ + buffer_size_ - 1);
	return result ? 0 : EOF;
}

int os_filebuf::sync()
{
	return overflow() == EOF ? -1 : 0;
}

int os_filebuf::underflow()
{
	DWORD actual;
	BOOL result = ::ReadFile(handle(), gbuf_, buffer_size_, &actual, NULL);
	if (!result || actual == 0)
		return EOF;
	setg(gbuf_, gbuf_, gbuf_ + actual);
	return static_cast<unsigned char>(*gbuf_);
}

}
