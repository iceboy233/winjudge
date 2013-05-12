#ifndef _DEBUG_OBJECT_HPP
#define _DEBUG_OBJECT_HPP

#include <windows.h>
extern "C" {
	#include <ntndk.h>
}

namespace judge {

class process;

class debug_object {
public:
	debug_object(bool kill_on_close);
	virtual ~debug_object();
	void close();
	HANDLE handle();
	void attach(HANDLE process_handle);
	bool wait_for_event();

private:
	// non-copyable
	debug_object(const debug_object &);
	debug_object &operator=(const debug_object &);

private:
	HANDLE _create(bool kill_on_close);

protected:
	// The return value is used as continue status, all handles are borrowed-semantic
	// and will be closed after return.

	virtual NTSTATUS on_create_thread(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

	virtual NTSTATUS on_create_process(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

	virtual NTSTATUS on_exit_thread(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

	virtual NTSTATUS on_exit_process(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

	virtual NTSTATUS on_exception(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

	virtual NTSTATUS on_load_dll(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

	virtual NTSTATUS on_unload_dll(PDBGUI_WAIT_STATE_CHANGE state_change)
		{ return DBG_CONTINUE; }

private:
	HANDLE handle_;
};

}

#endif
