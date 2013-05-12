#ifndef _THREAD_SUSPENSION
#define _THREAD_SUSPENSION

#include <windows.h>

namespace judge {

class thread_suspension {
public:
	thread_suspension(HANDLE handle);
	~thread_suspension();

private:
	// non-copyable
	thread_suspension(const thread_suspension &);
	thread_suspension &operator=(const thread_suspension &);

private:
	HANDLE handle;
};

}

#endif
