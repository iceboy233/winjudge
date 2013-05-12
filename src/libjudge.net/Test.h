#pragma once

#include "Pool.h"
#include "Compiler.h"
#include "Fs.h"
#include "JudgeLimit.h"
#include "StepResult.h"

namespace Judge {

public ref class Test {
public:
	Test(Pool ^pool, Compiler ^compiler, JudgeFs::Fs ^sourceFs, System::String ^sourcePath);
	~Test() { this->!Test(); }
	!Test();
	void AddTestcase(JudgeFs::Fs ^dataFs, System::String ^inputPath, System::String ^outputPath, JudgeLimit ^limit);
	void AddTestcaseSpj(JudgeFs::Fs ^dataFs, System::String ^spjPrefix, System::String ^spjPathRel, System::String ^spjParam,
		JudgeLimit ^limit, JudgeLimit ^spjLimit);
	StepResult ^Step();

private:
	struct judge_test *test_;
};

}
