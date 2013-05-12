#include <string>
#include <windows.h>
#include "window_station.hpp"
#include "exception.hpp"

using namespace std;

namespace judge {

window_station::window_station()
	: handle_(_process_window_station()), has_name_(false)
{}

HANDLE window_station::_process_window_station()
{
	HANDLE result = ::GetProcessWindowStation();
	if (!result) {
		throw win32_exception(::GetLastError());
	}
	return result;
}

HANDLE window_station::handle()
{
	return handle_;
}

std::string window_station::name()
{
	if (!has_name_) {
		name_ = user_object::name();
		has_name_ = true;
	}

	return name_;
}

}
