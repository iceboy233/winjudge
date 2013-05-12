#include <judge.h>
#include <msclr/marshal.h>
#include "Compiler.h"
#include "JudgeLimit.h"
#include "JudgeException.h"

using namespace System;
using namespace msclr::interop;

namespace Judge {

Compiler::Compiler(String ^executablePath, String ^commandLine,
	String ^sourceFilename, String ^targetFilename, JudgeLimit ^compilerLimit,
	String ^targetExecutablePath, String ^targetCommandLine)
	: compiler_(nullptr)
{
	marshal_context mc;
	struct judge_compiler *result;
	struct judge_limit limit;

	if (compilerLimit) {
		limit.time_limit_ms = compilerLimit->TimeLimitMs;
		limit.memory_limit_kb = compilerLimit->MemoryLimitKb;
		limit.active_process_limit = compilerLimit->ActiveProcessLimit;
		limit.stderr_output_limit = compilerLimit->StderrOutputLimit;
	}

	jstatus_t status = judge_create_compiler(&result,
		mc.marshal_as<const char *>(executablePath),
		mc.marshal_as<const char *>(commandLine),
		mc.marshal_as<const char *>(sourceFilename),
		mc.marshal_as<const char *>(targetFilename),
		compilerLimit ? &limit : nullptr,
		mc.marshal_as<const char *>(targetExecutablePath),
		mc.marshal_as<const char *>(targetCommandLine));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}

	compiler_ = result;
}

Compiler::!Compiler()
{
	if (compiler_) {
		judge_destroy_compiler(compiler_);
		compiler_ = nullptr;
	}
}

struct judge_compiler *Compiler::Handle()
{
	if (!compiler_) {
		throw gcnew ObjectDisposedException("Compiler");
	}
	return compiler_;
}

}
