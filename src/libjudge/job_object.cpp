#include <cstdint>
#include <windows.h>
#include "job_object.hpp"
#include "process.hpp"
#include "exception.hpp"

using namespace std;

namespace judge {

job_object::job_object()
	: handle_(_create())
{}

job_object::~job_object()
{
	::CloseHandle(handle_);
}

HANDLE job_object::_create()
{
	HANDLE result = ::CreateJobObject(NULL, NULL);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
	return result;
}

void job_object::assign(HANDLE process_handle)
{
	BOOL result = ::AssignProcessToJobObject(handle(), process_handle);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

void job_object::terminate(int32_t exit_code)
{
	BOOL result = ::TerminateJobObject(handle(), static_cast<UINT>(exit_code));
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

HANDLE job_object::handle()
{
	return handle_;
}

job_object::limits_info job_object::limits()
{
	return limits_info(*this);
}

job_object::ui_restrictions_info job_object::ui_restrictions()
{
	return ui_restrictions_info(*this);
}

job_object::limits_info::limits_info(job_object &job)
	: job_(job)
{
	update();
}

void job_object::limits_info::update()
{
	BOOL result = ::QueryInformationJobObject(
		job_.handle(),
		::JobObjectExtendedLimitInformation,
		static_cast<::JOBOBJECT_EXTENDED_LIMIT_INFORMATION *>(this),
		sizeof(::JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
		NULL);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

void job_object::limits_info::commit()
{
	BOOL result = ::SetInformationJobObject(
		job_.handle(),
		::JobObjectExtendedLimitInformation,
		static_cast<::JOBOBJECT_EXTENDED_LIMIT_INFORMATION *>(this),
		sizeof(::JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

job_object::ui_restrictions_info::ui_restrictions_info(job_object &job)
	: job_(job)
{
	update();
}

void job_object::ui_restrictions_info::update()
{
	BOOL result = ::QueryInformationJobObject(
		job_.handle(),
		::JobObjectBasicUIRestrictions,
		static_cast<::JOBOBJECT_BASIC_UI_RESTRICTIONS *>(this),
		sizeof(::JOBOBJECT_BASIC_UI_RESTRICTIONS),
		NULL);
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

void job_object::ui_restrictions_info::commit()
{
	BOOL result = ::SetInformationJobObject(
		job_.handle(),
		::JobObjectBasicUIRestrictions,
		static_cast<::JOBOBJECT_BASIC_UI_RESTRICTIONS *>(this),
		sizeof(::JOBOBJECT_BASIC_UI_RESTRICTIONS));
	if (!result) {
		throw win32_exception(::GetLastError());
	}
}

}
