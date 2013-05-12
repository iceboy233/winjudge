#ifndef _PROCESS_HPP
#define _PROCESS_HPP

#include <cstdint>
#include <windows.h>

namespace judge {

class process {
public:
	process(HANDLE handle);
	~process();
	HANDLE handle();
	void terminate(std::int32_t exit_code);
	std::uint32_t id();
	void start_timer();
	std::uint32_t alive_time_ms();
	std::uint32_t peak_memory_usage_kb();
	uint32_t virtual_protect(void *address, size_t length, uint32_t new_protect);
	void read_memory(const void *address, void *buffer, size_t length);
	void write_memory(void *address, const void *buffer, size_t length);

private:
	// non-copyable
	process(const process &);
	process &operator=(const process &);

private:
	std::uint64_t _process_time();

private:
	HANDLE handle_;
	bool timer_started_;
	uint32_t processor_count_;
	uint64_t initial_process_time_;
	uint64_t initial_idle_time_;
};

}

#endif
