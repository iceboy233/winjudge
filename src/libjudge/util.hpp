#ifndef _UTIL_HPP
#define _UTIL_HPP

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <windows.h>

namespace judge {
namespace util {

typedef std::shared_ptr<std::remove_pointer<HANDLE>::type> safe_handle_t;

safe_handle_t make_safe_handle(HANDLE object);
std::uint32_t get_processor_count();
std::uint64_t get_tick_count();
std::uint64_t get_idle_time();

template <std::size_t buffer_size>
bool stream_copy(std::istream &in, std::ostream &out)
{
	char buffer[buffer_size];

	while (!in.eof()) {
		in.read(buffer, sizeof(buffer));
		if (in.bad()) return false;
		out.write(buffer, in.gcount());
		if (out.fail()) return false;
	}

	return true;
}

template <std::size_t buffer_size>
bool stream_copy_n(std::istream &in, std::ostream &out, std::size_t max_size)
{
	char buffer[buffer_size];
	std::size_t size;

	while (max_size && !in.eof()) {
		size = std::min(sizeof(buffer), max_size);
		in.read(buffer, size);
		if (in.bad()) return false;
		size = static_cast<std::size_t>(in.gcount());
		out.write(buffer, size);
		if (out.fail()) return false;
		max_size -= size;
	}

	return true;
}

}
}

#endif
