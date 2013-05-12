#pragma once

#include <judge.h>

namespace Judge {

public ref class Pool {
public:
	Pool(System::String ^tempDir);
	~Pool() { this->!Pool(); }
	!Pool();
	struct judge_pool *Handle();
	void AddEnv(System::String ^username, System::String ^password);
	void AddEnvUnsafe();

private:
	struct judge_pool *pool_;
};

}
