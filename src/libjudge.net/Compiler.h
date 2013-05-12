#pragma once

#include <judge.h>
#include "JudgeLimit.h"

namespace Judge {

public ref class Compiler {
public:
	Compiler(System::String ^executablePath, System::String ^commandLine,
		System::String ^sourceFilename, System::String ^targetFilename, JudgeLimit ^compilerLimit,
		System::String ^targetExecutablePath, System::String ^targetCommandLine);
	~Compiler() { this->!Compiler(); }
	!Compiler();
	struct judge_compiler *Handle();

private:
	struct judge_compiler *compiler_;
};

}
