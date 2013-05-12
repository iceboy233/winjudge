#include <stdexcept>
#include <windows.h>
extern "C" {
	#include <ntndk.h>
}
#include <judge.h>
#include "exception.hpp"

namespace judge {

judge_exception::judge_exception(jstatus_t status)
	: status_(status)
{}

jstatus_t judge_exception::status() const
{
	return status_;
}

const char *judge_exception::what() const
{
	switch (status_) {
	case JSTATUS_SUCCESS:
		return "Success";
	case JSTATUS_GENERIC_ERROR:
		return "Generic error";
	case JSTATUS_NOT_IMPLEMENTED:
		return "Not implemented";
	case JSTATUS_BAD_ALLOC:
		return "Bad allocation";
	case JSTATUS_CONCURRENCY_LIMIT_EXCEEDED:
		return "Concurrency limit exceeded";
	case JSTATUS_NOT_FOUND:
		return "Not found";
	case JSTATUS_LOGON_FAILURE:
		return "Logon failure";
	case JSTATUS_PRIVILEGE_NOT_HELD:
		return "Privilege not held";
	case JSTATUS_PARAMETER_TOO_LARGE:
		return "Parameter too large";
	case JSTATUS_NOT_SUPPORTED:
		return "Not supported";
	case JSTATUS_INCORRECT_HANDLE:
		return "Incorrect handle";
	default:
		return "Unnamed exception";
	}
}

win32_exception::win32_exception(DWORD error_code)
	: judge_exception(_error_code_to_jstatus(error_code))
{}

jstatus_t win32_exception::_error_code_to_jstatus(DWORD error_code)
{
	switch (error_code) {
	case ERROR_SUCCESS:
		return JSTATUS_SUCCESS;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return JSTATUS_NOT_FOUND;
	case ERROR_LOGON_FAILURE:
	case ERROR_ACCOUNT_RESTRICTION:
	case ERROR_INVALID_LOGON_HOURS:
	case ERROR_INVALID_WORKSTATION:
	case ERROR_PASSWORD_EXPIRED:
	case ERROR_ACCOUNT_DISABLED:
	case ERROR_PASSWORD_MUST_CHANGE:
		return JSTATUS_LOGON_FAILURE;
	case ERROR_PRIVILEGE_NOT_HELD:
		return JSTATUS_PRIVILEGE_NOT_HELD;
	default:
		return JSTATUS_GENERIC_ERROR;
	}
}

DWORD win32_exception::error_code() const
{
	return error_code_;
}

winnt_exception::winnt_exception(NTSTATUS status)
	: judge_exception(_ntstatus_to_jstatus(status))
{}

jstatus_t winnt_exception::_ntstatus_to_jstatus(NTSTATUS status)
{
	switch (status) {
	case STATUS_SUCCESS:
		return JSTATUS_SUCCESS;
	case STATUS_NOT_SUPPORTED:
		return JSTATUS_NOT_SUPPORTED;
	default:
		return JSTATUS_GENERIC_ERROR;
	}
}

}