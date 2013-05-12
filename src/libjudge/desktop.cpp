#include <string>
#include <windows.h>
#include "desktop.hpp"
#include "exception.hpp"

namespace judge {

desktop::desktop(const std::string &name)
	: handle_(_create_desktop(name)), name_(name)
{}

desktop::~desktop()
{
	::CloseDesktop(reinterpret_cast<HDESK>(handle()));
}

HANDLE desktop::handle()
{
	return handle_;
}

std::string desktop::name()
{
	return name_;
}

HANDLE desktop::_create_desktop(const std::string &name)
{
	HANDLE result = ::CreateDesktopA(name.c_str(), NULL, NULL, 0,
		DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_WRITEOBJECTS |
		READ_CONTROL | WRITE_DAC | DESKTOP_SWITCHDESKTOP, 0);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
	return result;
}

}
