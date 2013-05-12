#include <judgefs.h>
#include <msclr/marshal.h>
#include "RamFs.h"
#include "JudgeException.h"

using namespace System;
using namespace Judge;
using namespace msclr::interop;

namespace JudgeFs {

RamFs::RamFs()
{
	struct judgefs *result;
	jstatus_t status = ramfs_create(&result);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
	Fs::SetHandle(result);
}

void RamFs::Set(String ^name, String ^value)
{
	marshal_context mc;
	jstatus_t status = ramfs_set(Fs::Handle(),
		mc.marshal_as<const char *>(name),
		mc.marshal_as<const char *>(value));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
}

}
