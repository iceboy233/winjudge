#include <judge.h>
#include <msclr/marshal.h>
#include "Test.h"
#include "Pool.h"
#include "Compiler.h"
#include "Fs.h"
#include "JudgeException.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;

namespace Judge {

Test::Test(Pool ^pool, Compiler ^compiler, JudgeFs::Fs ^sourceFs, String ^sourcePath)
	: test_(nullptr)
{
	marshal_context mc;
	struct judge_test *result;

	jstatus_t status = judge_create_test(&result, pool->Handle(), compiler->Handle(),
		sourceFs->Handle(), mc.marshal_as<const char *>(sourcePath));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}

	test_ = result;
}

Test::!Test()
{
	if (test_) {
		judge_destroy_test(test_);
		test_ = nullptr;
	}
}

void Test::AddTestcase(JudgeFs::Fs ^dataFs, System::String ^inputPath, System::String ^outputPath, JudgeLimit ^limit)
{
	marshal_context mc;
	struct judge_limit limit0;

	if (limit) {
		limit0.time_limit_ms = limit->TimeLimitMs;
		limit0.memory_limit_kb = limit->MemoryLimitKb;
		limit0.active_process_limit = limit->ActiveProcessLimit;
		limit0.stderr_output_limit = limit->StderrOutputLimit;
	}

	jstatus_t status = judge_add_testcase(test_, dataFs->Handle(),
		mc.marshal_as<const char *>(inputPath),
		mc.marshal_as<const char *>(outputPath),
		limit ? &limit0 : nullptr);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
}

void Test::AddTestcaseSpj(JudgeFs::Fs ^dataFs, System::String ^spjPrefix, System::String ^spjPathRel, System::String ^spjParam,
	JudgeLimit ^limit, JudgeLimit ^spjLimit)
{
	marshal_context mc;
	struct judge_limit limit0;
	struct judge_limit spj_limit0;

	if (limit) {
		limit0.time_limit_ms = limit->TimeLimitMs;
		limit0.memory_limit_kb = limit->MemoryLimitKb;
		limit0.active_process_limit = limit->ActiveProcessLimit;
		limit0.stderr_output_limit = limit->StderrOutputLimit;
	}

	if (spjLimit) {
		spj_limit0.time_limit_ms = spjLimit->TimeLimitMs;
		spj_limit0.memory_limit_kb = spjLimit->MemoryLimitKb;
		spj_limit0.active_process_limit = spjLimit->ActiveProcessLimit;
		spj_limit0.stderr_output_limit = spjLimit->StderrOutputLimit;
	}

	jstatus_t status = judge_add_testcase_spj(test_, dataFs->Handle(),
		mc.marshal_as<const char *>(spjPrefix),
		mc.marshal_as<const char *>(spjPathRel),
		mc.marshal_as<const char *>(spjParam),
		limit ? &limit0 : nullptr,
		spjLimit ? &spj_limit0 : nullptr);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
}

StepResult ^Test::Step()
{
	uint32_t phase, index;
	const struct judge_result *result;

	jstatus_t status = judge_step_test(test_, &phase, &index, &result);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}

	StepResult ^sr = gcnew StepResult;
	sr->Phase = static_cast<JudgePhase>(phase);
	sr->Index = index;
	if (phase != JUDGE_PHASE_NO_MORE) {
		sr->Flag = static_cast<JudgeFlag>(result->flag);
		sr->TimeUsageMs = result->time_usage_ms;
		sr->MemoryUsageKb = result->memory_usage_kb;
		sr->RuntimeError = static_cast<JudgeRuntimeError>(result->runtime_error);
		sr->JudgeOutput = marshal_as<String ^>(result->judge_output);
		sr->UserOutput = marshal_as<String ^>(result->user_output);
	}
	return sr;
}

}
