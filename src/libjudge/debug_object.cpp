#include <cassert>
#include <windows.h>
extern "C" {
	#include <ntndk.h>
}
#include "debug_object.hpp"
#include "process.hpp"
#include "exception.hpp"

using namespace std;

namespace {

class scoped_handle {
public:
	scoped_handle(HANDLE &handle)
		: handle_(handle)
	{}

	~scoped_handle()
	{
		if (handle_ != NULL)
			::NtClose(handle_);
	}

private:
	HANDLE &handle_;
};

class debug_continue {
public:
	debug_continue(HANDLE handle, CLIENT_ID &client_id, NTSTATUS initial_status)
		: handle_(handle), client_id_(client_id), status_(initial_status)
	{}

	~debug_continue()
	{
		::NtDebugContinue(handle_, &client_id_, status_);
	}

	void set_status(NTSTATUS status)
	{
		status_ = status;
	}

private:
	HANDLE handle_;
	CLIENT_ID &client_id_;
	NTSTATUS status_;
};

}

namespace judge {

debug_object::debug_object(bool kill_on_close)
	: handle_(_create(kill_on_close))
{}

HANDLE debug_object::_create(bool kill_on_close)
{
	HANDLE result;
	NTSTATUS status = ::NtCreateDebugObject(&result,
		DEBUG_OBJECT_ALL_ACCESS, NULL, kill_on_close ? TRUE : FALSE);
	if (!NT_SUCCESS(status)) {
		throw winnt_exception(status);
	}
	return result;
}

debug_object::~debug_object()
{
	close();
}

void debug_object::close()
{
	if (handle_) {
		::NtClose(handle_);
		handle_ = NULL;
	}
}

HANDLE debug_object::handle()
{
	return handle_;
}

void debug_object::attach(HANDLE process_handle)
{
	NTSTATUS status = ::NtDebugActiveProcess(process_handle, handle());
	if (!NT_SUCCESS(status)) {
		throw winnt_exception(status);
	}
}

bool debug_object::wait_for_event()
{
	DBGUI_WAIT_STATE_CHANGE state_change;
	NTSTATUS status = ::NtWaitForDebugEvent(handle(), FALSE, NULL, &state_change);
	if (status == STATUS_DEBUGGER_INACTIVE)
		return false;
	if (!NT_SUCCESS(status)) {
		throw winnt_exception(status);
	}

	debug_continue cont(handle(), state_change.AppClientId, DBG_CONTINUE);
	switch (state_change.NewState) {
	case DbgCreateThreadStateChange:
		{
			scoped_handle h0(state_change.StateInfo.CreateThread.HandleToThread);
			cont.set_status(on_create_thread(&state_change));
		}
		break;
	case DbgCreateProcessStateChange:
		{
			scoped_handle h0(state_change.StateInfo.CreateProcessInfo.HandleToProcess);
			scoped_handle h1(state_change.StateInfo.CreateProcessInfo.HandleToThread);
			scoped_handle h2(state_change.StateInfo.CreateProcessInfo.NewProcess.FileHandle);
			cont.set_status(on_create_process(&state_change));
		}
		break;
	case DbgExitThreadStateChange:
		cont.set_status(on_exit_thread(&state_change));
		break;
	case DbgExitProcessStateChange:
		cont.set_status(on_exit_process(&state_change));
		break;
	case DbgExceptionStateChange:
	case DbgBreakpointStateChange:
	case DbgSingleStepStateChange:
		cont.set_status(DBG_EXCEPTION_NOT_HANDLED);
		cont.set_status(on_exception(&state_change));
		break;
	case DbgLoadDllStateChange:
		{
			scoped_handle h0(state_change.StateInfo.LoadDll.FileHandle);
			cont.set_status(on_load_dll(&state_change));
		}
		break;
	case DbgUnloadDllStateChange:
		cont.set_status(on_unload_dll(&state_change));
		break;
	default:
		assert(!"unknown debug state");
	}

	return true;
}

}
