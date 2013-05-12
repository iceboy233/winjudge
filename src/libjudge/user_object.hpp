#ifndef _USER_OBJECT_HPP
#define _USER_OBJECT_HPP

#include <string>
#include <vector>
#include <windows.h>

namespace judge {

class user_object {
public:
	struct allowed_ace {
		allowed_ace(BYTE flags, ACCESS_MASK mask)
			: flags(flags)
			, mask(mask)
		{}

		BYTE flags;
		ACCESS_MASK mask;
	};

public:
	virtual ~user_object() {}
	virtual HANDLE handle() = 0;
	virtual std::string name();
	void add_allowed_ace(const std::vector<char> &sid, const allowed_ace &ace);
	void remove_ace_by_sid(const std::vector<char> &sid);
};

}

#endif
