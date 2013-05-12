#include <cstdint>
#include <vector>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/spin_mutex.hpp>
#include <winstl/shims/conversion/to_uint64.hpp>
#include <windows.h>
extern "C" {
	#include <ntndk.h>
}
#include "exception.hpp"
#include "util.hpp"

using namespace std;
using stlsoft::lock_scope;
using winstl::spin_mutex;
using stlsoft::to_uint64;

namespace {

ULARGE_INTEGER tick_count_prev_ = {0};
spin_mutex tick_count_mutex_;

}

namespace judge {
namespace util {

safe_handle_t make_safe_handle(HANDLE object)
{
	return safe_handle_t(object, ::CloseHandle);
}

uint32_t get_processor_count()
{
	SYSTEM_INFO system_info;
	::GetSystemInfo(&system_info);
	return static_cast<uint32_t>(system_info.dwNumberOfProcessors);
}

uint64_t get_tick_count()
{
	unsigned long tick_lo = ::GetTickCount();

	lock_scope<spin_mutex> lock(tick_count_mutex_);
	if (tick_lo < tick_count_prev_.LowPart) {
		++tick_count_prev_.HighPart;
	}
	tick_count_prev_.LowPart = tick_lo;
	return to_uint64(tick_count_prev_);
}

uint64_t get_idle_time()
{
	const size_t buffer_length = 328;
	vector<char> buffer(buffer_length);
	PSYSTEM_PERFORMANCE_INFORMATION spi = reinterpret_cast<PSYSTEM_PERFORMANCE_INFORMATION>(&*buffer.begin());

	NTSTATUS status = ::NtQuerySystemInformation(SystemPerformanceInformation, spi, buffer_length, NULL);
	if (!NT_SUCCESS(status)) {
		throw winnt_exception(status);
	}

	return to_uint64(reinterpret_cast<ULARGE_INTEGER &>(spi->IdleProcessTime));
}

}
}
