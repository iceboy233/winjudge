#ifndef _RANDOM_STRING_HPP
#define _RANDOM_STRING_HPP

#include <string>
#include <cstddef>

namespace judge {

class random_string : public std::string {
public:
	random_string(std::size_t size, const std::string &base = "abcdefghijklmnopqrstuvwxyz0123456789");

private:
	static std::string _make_string(std::size_t size, const std::string &base);
};

}

#endif
