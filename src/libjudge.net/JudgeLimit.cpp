#include "JudgeLimit.h"

using namespace System;

namespace Judge {

JudgeLimit::JudgeLimit()
	: TimeLimitMs(0), MemoryLimitKb(0)
	, ActiveProcessLimit(0), StderrOutputLimit(0)
{}

JudgeLimit::JudgeLimit(UInt32 TimeLimitMs, UInt32 MemoryLimitKb,
	UInt32 ActiveProcessLimit, UInt32 StderrOutputLimit)
	: TimeLimitMs(TimeLimitMs), MemoryLimitKb(MemoryLimitKb)
	, ActiveProcessLimit(ActiveProcessLimit), StderrOutputLimit(StderrOutputLimit)
{}

}
