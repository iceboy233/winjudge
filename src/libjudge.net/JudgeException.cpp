#include <judge.h>
#include "JudgeException.h"

namespace Judge {

JudgeException::JudgeException(jstatus_t status)
	: System::Exception(_to_message(status)), status_(status)
{}

jstatus_t JudgeException::StatusCode()
{
	return status_;
}

System::String ^JudgeException::_to_message(jstatus_t status)
{
	switch (status) {
	case JSTATUS_SUCCESS:
		return "JSTATUS_SUCCESS";
	case JSTATUS_GENERIC_ERROR:
		return "JSTATUS_GENERIC_ERROR";
	case JSTATUS_NOT_IMPLEMENTED:
		return "JSTATUS_NOT_IMPLEMENTED";
	case JSTATUS_BAD_ALLOC:
		return "JSTATUS_BAD_ALLOC";
	case JSTATUS_CONCURRENCY_LIMIT_EXCEEDED:
		return "JSTATUS_CONCURRENCY_LIMIT_EXCEEDED";
	case JSTATUS_NOT_FOUND:
		return "JSTATUS_NOT_FOUND";
	case JSTATUS_LOGON_FAILURE:
		return "JSTATUS_LOGON_FAILURE";
	case JSTATUS_PRIVILEGE_NOT_HELD:
		return "JSTATUS_PRIVILEGE_NOT_HELD";
	case JSTATUS_PARAMETER_TOO_LARGE:
		return "JSTATUS_PARAMETER_TOO_LARGE";
	case JSTATUS_NOT_SUPPORTED:
		return "JSTATUS_NOT_SUPPORTED";
	case JSTATUS_INCORRECT_HANDLE:
		return "JSTATUS_INCORRECT_HANDLE";
	default:
		return "Unknown error";
	}
}

}
