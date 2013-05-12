#ifndef _JOB_OBJECT_HPP
#define _JOB_OBJECT_HPP

#include <cstdint>
#include <windows.h>

namespace judge {

class process;

class job_object {
public:
	class limits_info : public ::JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
	public:
		limits_info(job_object &job);
		void update();
		void commit();
	private:
		job_object &job_;
	};

	class ui_restrictions_info : public ::JOBOBJECT_BASIC_UI_RESTRICTIONS {
	public:
		ui_restrictions_info(job_object &job);
		void update();
		void commit();
	private:
		job_object &job_;
	};

public:
	job_object();
	~job_object();
	void assign(HANDLE process_handle);
	void terminate(std::int32_t exit_code);
	HANDLE handle();
	limits_info limits();
	ui_restrictions_info ui_restrictions();

private:
	// non-copyable
	job_object(const job_object &);
	job_object &operator=(const job_object &);

private:
	HANDLE _create();

private:
	HANDLE handle_;
};

}

#endif
