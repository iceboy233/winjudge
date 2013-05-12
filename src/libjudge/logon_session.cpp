#include <string>
#include <vector>
#include <windows.h>
#include "logon_session.hpp"
#include "exception.hpp"

using namespace std;

namespace judge {

logon_session::logon_session(const std::string &username, const std::string &password)
	: handle_(_logon_user(username, password))
{}

HANDLE logon_session::_logon_user(const string &username, const string &password)
{
	HANDLE result;
	BOOL success = LogonUserA(username.c_str(), ".", password.c_str(),
		LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &result);
	if (!success) {
		throw win32_exception(::GetLastError());
	}
	return result;
}

logon_session::~logon_session()
{
	::CloseHandle(handle());
}

HANDLE logon_session::handle()
{
	return handle_;
}

vector<char> logon_session::sid()
{
	vector<char> buffer;
	PTOKEN_GROUPS ptg;
	DWORD length;
	DWORD index;

	if (!::GetTokenInformation(handle(), TokenGroups, NULL, NULL, &length)) {
		DWORD error_code = ::GetLastError();
		if (error_code != ERROR_INSUFFICIENT_BUFFER) {
			throw win32_exception(error_code);
		}
	} else {
		length = 0;
	}

	if (!length) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}

	buffer.resize(length);
	ptg = reinterpret_cast<PTOKEN_GROUPS>(&*buffer.begin());
	if (!::GetTokenInformation(handle(), TokenGroups, ptg, length, &length)) {
		throw win32_exception(::GetLastError());
	}

	for (index = 0; index < ptg->GroupCount; ++index) {
		if ((ptg->Groups[index].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
			length = ::GetLengthSid(ptg->Groups[index].Sid);
			if (!length) {
				throw judge_exception(JSTATUS_GENERIC_ERROR);
			}
			vector<char> sid(length);
			if (!::CopySid(length, &*sid.begin(), ptg->Groups[index].Sid)) {
				throw win32_exception(::GetLastError());
			}
			return sid;
		}
	}

	// token group not found
	throw judge_exception(JSTATUS_GENERIC_ERROR);
}

}
