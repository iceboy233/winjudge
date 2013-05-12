#ifndef _EXCEPTION_HPP
#define _EXCEPTION_HPP

#include <stdexcept>
#include <windows.h>
extern "C" {
	#include <ntndk.h>
}
#include <judge.h>

namespace judge {

class judge_exception : public std::exception {
public:
	explicit judge_exception(jstatus_t status);
	jstatus_t status() const;
	virtual const char *what() const;

private:
	jstatus_t status_;
};

class win32_exception : public judge_exception {
public:
	explicit win32_exception(DWORD error_code);
	DWORD error_code() const;

private:
	static jstatus_t _error_code_to_jstatus(DWORD error_code);

private:
	DWORD error_code_;
};

class winnt_exception : public judge_exception {
public:
	explicit winnt_exception(NTSTATUS status);
	NTSTATUS status() const;

private:
	static jstatus_t _ntstatus_to_jstatus(NTSTATUS status);

private:
	NTSTATUS status_;
};

}

#endif
