#ifndef _LOGON_SESSION_HPP
#define _LOGON_SESSION_HPP

#include <string>
#include <vector>
#include <windows.h>

namespace judge {

class logon_session {
public:
	logon_session(const std::string &username, const std::string &password);
	~logon_session();
	HANDLE handle();
	std::vector<char> sid();

private:
	static HANDLE _logon_user(const std::string &username, const std::string &password);

private:
	HANDLE handle_;
};

}

#endif
