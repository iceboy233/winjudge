#ifndef _WINDOW_STATION_HPP
#define _WINDOW_STATION_HPP

#include <string>
#include <windows.h>
#include "user_object.hpp"

namespace judge {

class window_station : public user_object {
public:
	window_station();
	/* override */ HANDLE handle();
	/* override */ std::string name();

private:
	static HANDLE _process_window_station();

private:
	HANDLE handle_;
	bool has_name_;
	std::string name_;
};

}

#endif
