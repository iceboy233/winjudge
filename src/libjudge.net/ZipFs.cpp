#include <judgefs.h>
#include <msclr/marshal.h>
#include "ZipFs.h"
#include "JudgeException.h"

using namespace System;
using namespace Judge;
using namespace msclr::interop;

namespace JudgeFs {

ZipFs::ZipFs(String ^path)
{
	marshal_context mc;
	struct judgefs *result;
	jstatus_t status = zipfs_create(&result, mc.marshal_as<const char *>(path));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
	Fs::SetHandle(result);
}

}
