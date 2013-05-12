#pragma once

#include <judge.h>

namespace Judge {

public enum class JudgePhase {
	NoMore = JUDGE_PHASE_NO_MORE,
	Compile = JUDGE_PHASE_COMPILE,
	Testcase = JUDGE_PHASE_TESTCASE,
	Summary = JUDGE_PHASE_SUMMARY,
};

public enum class JudgeFlag {
	None = JUDGE_NONE,
	Accepted = JUDGE_ACCEPTED,
	WrongAnswer = JUDGE_WRONG_ANSWER,
	RuntimeError = JUDGE_RUNTIME_ERROR,
	TimeLimitExceeded = JUDGE_TIME_LIMIT_EXCEEDED,
	MemoryLimitExceeded = JUDGE_MEMORY_LIMIT_EXCEEDED,
	CompileError = JUDGE_COMPILE_ERROR,
};

public enum class JudgeRuntimeError {
	Unknown = RUNTIME_UNKNOWN,
	StartFailed = RUNTIME_START_FAILED,
	ReadAccessViolation = RUNTIME_READ_ACCESS_VIOLATION,
	WriteAccessViolation = RUNTIME_WRITE_ACCESS_VIOLATION,
	ExecuteAccessViolation = RUNTIME_EXECUTE_ACCESS_VIOLATION,
	DivideByZero = RUNTIME_DIVIDE_BY_ZERO,
	StackOverflow = RUNTIME_STACK_OVERFLOW,
};
	
public ref class StepResult {
public:
	JudgePhase Phase;
	System::UInt32 Index;
	JudgeFlag Flag;
	System::UInt32 TimeUsageMs;
	System::UInt32 MemoryUsageKb;
	JudgeRuntimeError RuntimeError;
	System::String ^JudgeOutput;
	System::String ^UserOutput;
};

}
