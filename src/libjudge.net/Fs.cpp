#include <judgefs.h>
#include <msclr/marshal.h>
#include "Fs.h"
#include "JudgeException.h"

using namespace System;
using namespace System::Collections;
using namespace System::Runtime::InteropServices;
using namespace Judge;
using namespace msclr::interop;

namespace JudgeFs {

Fs::Fs()
	: fs_(nullptr)
{}

Fs::!Fs()
{
	if (fs_) {
		judgefs_release(fs_);
		fs_ = nullptr;
	}
}

struct judgefs *Fs::Handle()
{
	if (!fs_) {
		throw gcnew ObjectDisposedException("Fs");
	}
	return fs_;
}

void Fs::SetHandle(struct judgefs *fs)
{
	fs_ = fs;
}

FsFindResult ^Fs::Find(System::String ^prefix)
{
	return gcnew FsFindResult(this, prefix);
}

FsStream ^Fs::Open(System::String ^path)
{
	return gcnew FsStream(this, path);
}

FsFindResult::FsFindResult(Fs ^fs, System::String ^prefix)
	: fs_(fs), prefix_(prefix)
{}

IEnumerator ^FsFindResult::GetEnumerator()
{
	return gcnew Enumerator(fs_, prefix_);
}

FsFindResult::Enumerator::Enumerator(Fs ^fs, System::String ^prefix)
	: fs_(fs->Handle()), prefix_(mc_.marshal_as<const char *>(prefix)), context_(nullptr), end_(false)
{}

FsFindResult::Enumerator::!Enumerator()
{
	Reset();
}

void FsFindResult::Enumerator::Reset()
{
	while (current_) {
		struct judgefs *fs = fs_;
		const char *prefix = prefix_;
		void *context = context_;
		const char *current;
		jstatus_t status = fs->vtable->find(fs, &current, prefix, &context);
		if (!JSUCCESS(status)) {
			throw gcnew JudgeException(status);
		}
		current_ = current;
		context_ = context;
	}
	end_ = false;
}

bool FsFindResult::Enumerator::MoveNext()
{
	if (end_) {
		return false;
	}

	struct judgefs *fs = fs_;
	const char *prefix = prefix_;
	void *context = context_;
	const char *current;
	jstatus_t status = fs->vtable->find(fs, &current, prefix, &context);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
	current_ = current;
	context_ = context;

	if (current_) {
		return true;
	} else {
		end_ = true;
		return false;
	}
}

System::Object ^FsFindResult::Enumerator::Current::get()
{
	if (current_) {
		return marshal_as<System::String ^>(current_);
	} else {
		return nullptr;
	}
}

FsStream::FsStream(Fs ^fs, System::String ^path)
	: fs_(fs), file_(nullptr)
{
	marshal_context mc;
	struct judgefs *fsp = fs_->Handle();
	struct judgefs_file *file;
	jstatus_t status = fsp->vtable->open(fsp, &file, mc.marshal_as<const char *>(path));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
	file_ = file;
}

FsStream::!FsStream()
{
	if (file_) {
		struct judgefs *fs = fs_->Handle();
		fs->vtable->close(fs, file_);
		file_ = nullptr;
	}
}

int FsStream::Read(array<unsigned char>^ buffer, int offset, int count)
{
	GCHandle handle = GCHandle::Alloc(buffer, GCHandleType::Pinned);
	IntPtr p = Marshal::UnsafeAddrOfPinnedArrayElement(buffer, offset);
	struct judgefs *fs = fs_->Handle();
	struct judgefs_file *file = file_;
	uint32_t actual;
	jstatus_t status = fs->vtable->read(fs, file, static_cast<char *>(p.ToPointer()), count, &actual);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
	return actual;
}

}
