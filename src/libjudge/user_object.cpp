#include <string>
#include <vector>
#include <cstddef>
#include <windows.h>
#include "user_object.hpp"
#include "exception.hpp"

using namespace std;

namespace judge {

string user_object::name()
{
	vector<char> buffer;
	DWORD length;
	
	if (!::GetUserObjectInformationA(handle(), UOI_NAME, NULL, 0, &length)) {
		DWORD error_code = ::GetLastError();
		if (error_code != ERROR_INSUFFICIENT_BUFFER) {
			throw win32_exception(error_code);
		}
	} else {
		length = 0;
	}

	buffer.resize(length);
	if (!::GetUserObjectInformationA(handle(), UOI_NAME, &*buffer.begin(), length, &length)) {
		throw win32_exception(::GetLastError());
	}

	return string(&*buffer.begin());
}

void user_object::add_allowed_ace(const std::vector<char> &sid, const allowed_ace &ace)
{
	SECURITY_INFORMATION info_required = DACL_SECURITY_INFORMATION;
	DWORD length;

	if (!::GetUserObjectSecurity(handle(), &info_required, NULL, 0, &length)) {
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

	BOOL dacl_present;
	PACL acl_old;
	BOOL dacl_defaulted;
	vector<char> sd_buffer(length);
	PSECURITY_DESCRIPTOR psd = static_cast<PSECURITY_DESCRIPTOR>(&*sd_buffer.begin());
	if (!::GetUserObjectSecurity(handle(), &info_required, psd, length, &length)) {
		throw win32_exception(::GetLastError());
	}

	if (!::GetSecurityDescriptorDacl(psd, &dacl_present, &acl_old, &dacl_defaulted)) {
		throw win32_exception(::GetLastError());
	}

	size_t acl_new_size = sizeof(ACE_HEADER) + sizeof(DWORD) + sid.size();
	ACL_SIZE_INFORMATION acl_size_info;

	if (dacl_present) {
		if (!::GetAclInformation(acl_old, &acl_size_info, sizeof(acl_size_info), AclSizeInformation)) {
			throw win32_exception(::GetLastError());
		}

		acl_new_size += acl_size_info.AclBytesInUse;
	}

	vector<char> acl_new_buffer(acl_new_size);
	PACL acl_new = reinterpret_cast<PACL>(&*acl_new_buffer.begin());

	if (!::InitializeAcl(acl_new, static_cast<DWORD>(acl_new_size), ACL_REVISION)) {
		throw win32_exception(::GetLastError());
	}

	if (dacl_present) {
		for (DWORD index = 0; index < acl_size_info.AceCount; ++index) {
			PACE_HEADER temp_ace;
			if (!::GetAce(acl_old, index, reinterpret_cast<LPVOID *>(&temp_ace))) {
				throw win32_exception(::GetLastError());
			}
			if (!::AddAce(acl_new, ACL_REVISION, -1, temp_ace, temp_ace->AceSize)) {
				throw win32_exception(::GetLastError());
			}
		}
	}

	if (!::AddAccessAllowedAceEx(acl_new, ACL_REVISION, ace.flags, ace.mask,
		reinterpret_cast<PSID>(const_cast<char *>(&*sid.begin())))) {
		throw win32_exception(::GetLastError());
	}

	vector<char> sd_new_buffer(length);
	PSECURITY_DESCRIPTOR sd_new = reinterpret_cast<PSECURITY_DESCRIPTOR>(&*sd_new_buffer.begin());
	if (!::InitializeSecurityDescriptor(sd_new, SECURITY_DESCRIPTOR_REVISION)) {
		throw win32_exception(::GetLastError());
	}
	if (!::SetSecurityDescriptorDacl(sd_new, TRUE, acl_new, FALSE)) {
		throw win32_exception(::GetLastError());
	}
	if (!::SetUserObjectSecurity(handle(), &info_required, sd_new)) {
		throw win32_exception(::GetLastError());
	}
}

void user_object::remove_ace_by_sid(const std::vector<char> &sid)
{
	SECURITY_INFORMATION info_required = DACL_SECURITY_INFORMATION;
	DWORD length;

	if (!::GetUserObjectSecurity(handle(), &info_required, NULL, 0, &length)) {
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

	BOOL dacl_present;
	PACL acl_old;
	BOOL dacl_defaulted;
	vector<char> sd_buffer(length);
	PSECURITY_DESCRIPTOR psd = static_cast<PSECURITY_DESCRIPTOR>(&*sd_buffer.begin());
	if (!::GetUserObjectSecurity(handle(), &info_required, psd, length, &length)) {
		throw win32_exception(::GetLastError());
	}

	if (!::GetSecurityDescriptorDacl(psd, &dacl_present, &acl_old, &dacl_defaulted)) {
		throw win32_exception(::GetLastError());
	}

	if (dacl_present) {
		ACL_SIZE_INFORMATION acl_size_info;
		if (!::GetAclInformation(acl_old, &acl_size_info, sizeof(acl_size_info), AclSizeInformation)) {
			throw win32_exception(::GetLastError());
		}
		vector<char> acl_new_buffer(acl_size_info.AclBytesInUse);
		PACL acl_new = reinterpret_cast<PACL>(&*acl_new_buffer.begin());
		if (!::InitializeAcl(acl_new, acl_size_info.AclBytesInUse, ACL_REVISION)) {
			throw win32_exception(::GetLastError());
		}

		for (DWORD index = 0; index < acl_size_info.AceCount; ++index) {
			// use ACCESS_ALLOWED_ACE since all aces share the same strcture
			PACCESS_ALLOWED_ACE temp_ace;
			if (!::GetAce(acl_old, index, reinterpret_cast<LPVOID *>(&temp_ace))) {
				throw win32_exception(::GetLastError());
			}

			if (!::EqualSid(&temp_ace->SidStart, const_cast<char *>(&*sid.begin()))) {
				if (!::AddAce(acl_new, ACL_REVISION, -1, temp_ace, temp_ace->Header.AceSize)) {
					throw win32_exception(::GetLastError());
				}
			}
		}

		vector<char> sd_new_buffer(length);
		PSECURITY_DESCRIPTOR sd_new = reinterpret_cast<PSECURITY_DESCRIPTOR>(&*sd_new_buffer.begin());
		if (!::InitializeSecurityDescriptor(sd_new, SECURITY_DESCRIPTOR_REVISION)) {
			throw win32_exception(::GetLastError());
		}
		if (!::SetSecurityDescriptorDacl(sd_new, TRUE, acl_new, FALSE)) {
			throw win32_exception(::GetLastError());
		}
		if (!::SetUserObjectSecurity(handle(), &info_required, sd_new)) {
			throw win32_exception(::GetLastError());
		}
	}
}

}
