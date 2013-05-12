#include <windows.h>
#include "thread_suspension.hpp"

namespace judge {

thread_suspension::thread_suspension(HANDLE handle)
	: handle(handle)
{}

thread_suspension::~thread_suspension()
{
	::ResumeThread(handle);
	::CloseHandle(handle);
}

}
