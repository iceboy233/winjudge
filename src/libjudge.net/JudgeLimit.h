#pragma once

namespace Judge {

public ref class JudgeLimit {
public:
	JudgeLimit();
	JudgeLimit(System::UInt32 TimeLimitMs, System::UInt32 MemoryLimitKb,
		System::UInt32 ActiveProcessLimit, System::UInt32 StderrOutputLimit);

	System::UInt32 TimeLimitMs;
	System::UInt32 MemoryLimitKb;
	System::UInt32 ActiveProcessLimit;
	System::UInt32 StderrOutputLimit;
};

}
