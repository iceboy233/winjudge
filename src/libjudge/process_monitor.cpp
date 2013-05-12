#include <cassert>
#include <cstdint>
#include <memory>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <windows.h>
#include <process.h>
#include "process_monitor.hpp"
#include "process.hpp"
#include "exception.hpp"

using namespace std;
using stlsoft::lock_scope;
using winstl::thread_mutex;

namespace judge {

process_monitor::debugger::debugger(process_monitor &pm)
	: debug_object(true), pm_(pm)
{}

shared_ptr<process_monitor::context_base> process_monitor::debugger::_lookup_context(
	uint32_t process_id, bool erase)
{
	lock_scope<thread_mutex> lock(pm_.contexts_mutex_);
	auto iter = pm_.contexts_.find(process_id);
	if (iter == pm_.contexts_.end())
		return nullptr;
	shared_ptr<context_base> result = iter->second;
	if (erase) {
		pm_.contexts_.erase(iter);
	}
	return result;
}

NTSTATUS process_monitor::debugger::on_create_process(PDBGUI_WAIT_STATE_CHANGE state_change)
{
	auto thread = shared_ptr<remove_pointer<HANDLE>::type>(
		state_change->StateInfo.CreateProcessInfo.HandleToThread, ::CloseHandle);
	state_change->StateInfo.CreateProcessInfo.HandleToThread = NULL;
	DBGKM_CREATE_PROCESS &new_process = state_change->StateInfo.CreateProcessInfo.NewProcess;
	shared_ptr<context_base> context = _lookup_context(reinterpret_cast<uint32_t>(
		state_change->AppClientId.UniqueProcess));
	if (context) {
		context->add_thread(reinterpret_cast<uint32_t>(
			state_change->AppClientId.UniqueThread), thread);
		context->set_breakpoint_at_entry(new_process.BaseOfImage);
	}
	return DBG_CONTINUE;
}

NTSTATUS process_monitor::debugger::on_create_thread(PDBGUI_WAIT_STATE_CHANGE state_change)
{
	auto thread = shared_ptr<remove_pointer<HANDLE>::type>(
		state_change->StateInfo.CreateThread.HandleToThread, ::CloseHandle);
	state_change->StateInfo.CreateThread.HandleToThread = NULL;
	shared_ptr<context_base> context = _lookup_context(reinterpret_cast<uint32_t>(
		state_change->AppClientId.UniqueProcess));
	if (context) {
		context->add_thread(reinterpret_cast<uint32_t>(
			state_change->AppClientId.UniqueThread), thread);
	}
	return DBG_CONTINUE;
}

NTSTATUS process_monitor::debugger::on_exit_process(PDBGUI_WAIT_STATE_CHANGE state_change)
{
	shared_ptr<context_base> context = _lookup_context(reinterpret_cast<uint32_t>(
		state_change->AppClientId.UniqueProcess), true);
	if (!context) {
		return DBG_CONTINUE;
	}
	context->set_exit_status(static_cast<int32_t>(
		state_change->StateInfo.ExitProcess.ExitStatus));
	context->invoke();
	return DBG_CONTINUE;
}

NTSTATUS process_monitor::debugger::on_exception(PDBGUI_WAIT_STATE_CHANGE state_change)
{
	EXCEPTION_RECORD &record = state_change->StateInfo.Exception.ExceptionRecord;
	shared_ptr<context_base> context = _lookup_context(reinterpret_cast<uint32_t>(
		state_change->AppClientId.UniqueProcess));
	if (!context) {
		return DBG_EXCEPTION_NOT_HANDLED;
	}

	if (!state_change->StateInfo.Exception.FirstChance) {
		context->set_exception(shared_ptr<EXCEPTION_RECORD>(new EXCEPTION_RECORD(record)));
		try {
			context->job_object()->terminate(static_cast<int32_t>(record.ExceptionCode));
		} catch (const win32_exception &) {
		}
		return DBG_EXCEPTION_HANDLED;
	} else if (record.ExceptionCode == EXCEPTION_BREAKPOINT) {
		if (context->on_breakpoint(record.ExceptionAddress)) {
			auto thread = context->get_thread(reinterpret_cast<uint32_t>(state_change->AppClientId.UniqueThread));
			CONTEXT thread_context = {0};
			thread_context.ContextFlags = CONTEXT_CONTROL;
			::GetThreadContext(thread.get(), &thread_context);
			thread_context.Eip = reinterpret_cast<DWORD>(record.ExceptionAddress);
			::SetThreadContext(thread.get(), &thread_context);
			context->process()->start_timer();
			return DBG_EXCEPTION_HANDLED;
		}
		return DBG_EXCEPTION_HANDLED;
	} else {
		return DBG_EXCEPTION_NOT_HANDLED;
	}
}

process_monitor::context_base::context_base(const std::shared_ptr<judge::process> &ps,
	const std::shared_ptr<judge::job_object> &job)
	: ps_(ps), job_(job), entry_point_(NULL)
{}

void process_monitor::context_base::set_exit_status(int32_t exit_status)
{
	result_.exit_status = exit_status;
}

void process_monitor::context_base::set_exception(const shared_ptr<EXCEPTION_RECORD> &exception)
{
	result_.exception = exception;
}

const shared_ptr<judge::process> &process_monitor::context_base::process()
{
	return ps_;
}

const shared_ptr<judge::job_object> &process_monitor::context_base::job_object()
{
	return job_;
}

void process_monitor::context_base::add_thread(uint32_t thread_id,
	shared_ptr<remove_pointer<HANDLE>::type> thread_handle)
{
	threads_.insert(make_pair(thread_id, thread_handle));
}

shared_ptr<remove_pointer<HANDLE>::type> process_monitor::context_base::get_thread(
	uint32_t thread_id)
{
	auto iter = threads_.find(thread_id);
	if (iter == threads_.end()) {
		return nullptr;
	} else {
		return iter->second;
	}
}

void process_monitor::context_base::set_breakpoint_at_entry(void *base)
{
	IMAGE_DOS_HEADER dos_header;
	ps_->read_memory(base, &dos_header, sizeof(dos_header));
	IMAGE_FILE_HEADER file_header;
	PBYTE address = reinterpret_cast<PBYTE>(base) + dos_header.e_lfanew + sizeof(DWORD);
	ps_->read_memory(address, &file_header, sizeof(file_header));
	address += sizeof(IMAGE_FILE_HEADER);

	void *entry_point;
	if (file_header.Machine == IMAGE_FILE_MACHINE_I386) {
		IMAGE_OPTIONAL_HEADER32 opt_header;
		ps_->read_memory(address, &opt_header, sizeof(opt_header));
		entry_point = reinterpret_cast<void *>(reinterpret_cast<PBYTE>(base)
			+ opt_header.AddressOfEntryPoint);
	} else if (file_header.Machine == IMAGE_FILE_MACHINE_AMD64) {
		IMAGE_OPTIONAL_HEADER64 opt_header;
		ps_->read_memory(address, &opt_header, sizeof(opt_header));
		entry_point = reinterpret_cast<void *>(reinterpret_cast<PBYTE>(base)
			+ opt_header.AddressOfEntryPoint);
	} else {
		throw judge_exception(JSTATUS_NOT_SUPPORTED);
	}

	ps_->read_memory(entry_point, &stolen_byte_, sizeof(BYTE));
	BYTE trap_instruction = 0xcc;
	uint32_t old_protect = ps_->virtual_protect(entry_point, sizeof(BYTE), PAGE_EXECUTE_READWRITE);
	ps_->write_memory(entry_point, &trap_instruction, sizeof(BYTE));
	ps_->virtual_protect(entry_point, sizeof(BYTE), old_protect);
	entry_point_ = entry_point;
}

bool process_monitor::context_base::on_breakpoint(void *address)
{
	if (address != entry_point_) {
		return false;
	}

	uint32_t old_protect = ps_->virtual_protect(address, sizeof(BYTE), PAGE_EXECUTE_READWRITE);
	ps_->write_memory(address, &stolen_byte_, sizeof(BYTE));
	ps_->virtual_protect(address, sizeof(BYTE), old_protect);
	return true;
}

process_monitor::process_monitor()
#pragma warning(disable:4355)
	: debugger_(*this)
	, thread_handle_(_create_thread())
{}

process_monitor::~process_monitor()
{
	::CloseHandle(thread_handle_);
}

void process_monitor::rundown()
{
	debugger_.close();
	::WaitForSingleObject(thread_handle_, INFINITE);
}

HANDLE process_monitor::_create_thread()
{
	HANDLE result = reinterpret_cast<HANDLE>(
		::_beginthreadex(nullptr, 0, _thread_entry, this, 0, nullptr));
	if (!result) {
		throw judge_exception(JSTATUS_GENERIC_ERROR);
	}
	return result;
}

unsigned int __stdcall process_monitor::_thread_entry(void *param)
{
	process_monitor &pm = *reinterpret_cast<process_monitor *>(param);
	while (pm.debugger_.wait_for_event())
	{}
	return 0;
}

}
