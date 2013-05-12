#ifndef _PROC_THREAD_ATTRIBUTE_LIST
#define _PROC_THREAD_ATTRIBUTE_LIST

#include <cstdint>
#include <memory>
#include <vector>
#include <windows.h>

namespace judge {

class proc_thread_attribute_list_provider;

class proc_thread_attribute_list {
public:
	explicit proc_thread_attribute_list(size_t attr_count);
	~proc_thread_attribute_list();
	void update(uint32_t attr, void *value, size_t size);
	LPPROC_THREAD_ATTRIBUTE_LIST pointer();

private:
	// non-copyable
	proc_thread_attribute_list(const proc_thread_attribute_list &);
	proc_thread_attribute_list &operator=(const proc_thread_attribute_list &);

public:
	static bool is_supported();

private:
	static const std::shared_ptr<proc_thread_attribute_list_provider> &_provider();

private:
	std::vector<char> buffer_;
};

}

#endif
