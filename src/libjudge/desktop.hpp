#ifndef _DESKTOP_HPP
#define _DESKTOP_HPP

#include <string>
#include <windows.h>
#include "user_object.hpp"

namespace judge {

class desktop : public user_object {
public:
	desktop(const std::string &name);
	~desktop();
	/* override */ HANDLE handle();
	/* override */ std::string name();

private:
	static HANDLE _create_desktop(const std::string &name);

private:
	HANDLE handle_;
	std::string name_;
};

}

#endif
