#include <string>
#include <cstddef>
#include <windows.h>
#include "random_string.hpp"
#include "exception.hpp"

using namespace std;

namespace judge {

random_string::random_string(size_t size, const string &base)
	: string(_make_string(size, base))
{}

string random_string::_make_string(size_t size, const string &base)
{
	string result;
	HCRYPTPROV context;

	if (!CryptAcquireContextA(&context, NULL, MS_DEF_PROV_A, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		throw win32_exception(::GetLastError());
	}

	try {
		for (size_t i = 0; i != size; ++i) {
			uint32_t data;
			if (!::CryptGenRandom(context, sizeof(data), reinterpret_cast<BYTE *>(&data))) {
				throw win32_exception(::GetLastError());
			}
			result.push_back(base[data % base.size()]);
		}
		::CryptReleaseContext(context, 0);
		return result;
	} catch (...) {
		::CryptReleaseContext(context, 0);
		throw;
	}
}

}
