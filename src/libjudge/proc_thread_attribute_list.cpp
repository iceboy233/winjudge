#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include "proc_thread_attribute_list.hpp"
#include "exception.hpp"

using namespace std;
using stlsoft::lock_scope;
using winstl::thread_mutex;

namespace judge {

typedef BOOL WINAPI
Func_InitializeProcThreadAttributeList(
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    DWORD dwAttributeCount,
    DWORD dwFlags,
    PSIZE_T lpSize);

typedef BOOL WINAPI
Func_UpdateProcThreadAttribute(
	LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
	DWORD dwFlags,
	DWORD_PTR Attribute,
	PVOID lpValue,
	SIZE_T cbSize,
	PVOID lpPreviousValue,
	PSIZE_T lpReturnSize);

typedef VOID WINAPI
Func_DeleteProcThreadAttributeList(
	LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList);

class proc_thread_attribute_list_provider {
public:
	proc_thread_attribute_list_provider();
	bool is_supported()
		{ return is_supported_; }
	Func_InitializeProcThreadAttributeList *fn_init()
		{ return fn_init_; }
	Func_UpdateProcThreadAttribute *fn_update()
		{ return fn_update_; }
	Func_DeleteProcThreadAttributeList *fn_delete()
		{ return fn_delete_; }

private:
	bool is_supported_;
	Func_InitializeProcThreadAttributeList *fn_init_;
	Func_UpdateProcThreadAttribute *fn_update_;
	Func_DeleteProcThreadAttributeList *fn_delete_;
};

proc_thread_attribute_list_provider::proc_thread_attribute_list_provider()
	: is_supported_(false)
{
	OSVERSIONINFOA vi;

	vi.dwOSVersionInfoSize = sizeof(vi);
	if (!::GetVersionExA(&vi)) {
		throw win32_exception(::GetLastError());
	}

	if (vi.dwMajorVersion < 6) {
		return;
	}

	HMODULE mod = ::GetModuleHandleA("Kernel32.dll");
	if (!mod) {
		throw win32_exception(::GetLastError());
	}

	fn_init_ = reinterpret_cast<Func_InitializeProcThreadAttributeList *>(
		::GetProcAddress(mod, "InitializeProcThreadAttributeList"));
	fn_update_ = reinterpret_cast<Func_UpdateProcThreadAttribute *>(
		::GetProcAddress(mod, "UpdateProcThreadAttribute"));
	fn_delete_ = reinterpret_cast<Func_DeleteProcThreadAttributeList *>(
		::GetProcAddress(mod, "DeleteProcThreadAttributeList"));

	if (fn_init_ && fn_update_ && fn_delete_) {
		is_supported_ = true;
	}
}

const shared_ptr<proc_thread_attribute_list_provider> &proc_thread_attribute_list::_provider()
{
	static shared_ptr<proc_thread_attribute_list_provider> provider;
	static thread_mutex provider_mutex;

	if (!provider) {
		lock_scope<thread_mutex> lock(provider_mutex);
		if (!provider) {
			provider.reset(new proc_thread_attribute_list_provider);
		}
	}

	return provider;
}

bool proc_thread_attribute_list::is_supported()
{
	return _provider()->is_supported();
}

proc_thread_attribute_list::proc_thread_attribute_list(size_t attr_count)
{
	const shared_ptr<proc_thread_attribute_list_provider> &p = _provider();

	SIZE_T size = 0;
	if (!p->fn_init()(nullptr, attr_count, 0, &size)) {
		DWORD error_code = ::GetLastError();
		if (error_code != ERROR_INSUFFICIENT_BUFFER) {
			throw win32_exception(error_code);
		}
	}

	buffer_.resize(size);
	if (!p->fn_init()(pointer(), attr_count, 0, &size)) {
		throw win32_exception(::GetLastError());
	}
}

proc_thread_attribute_list::~proc_thread_attribute_list()
{
	_provider()->fn_delete()(pointer());
}

void proc_thread_attribute_list::update(uint32_t attr, void *value, size_t size)
{
	if (!_provider()->fn_update()(pointer(), 0, attr, value, size, nullptr, nullptr)) {
		throw win32_exception(::GetLastError());
	}
}

LPPROC_THREAD_ATTRIBUTE_LIST proc_thread_attribute_list::pointer()
{
	return reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer_.data());
}

}
