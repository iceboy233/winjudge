#include <judge.h>
#include <msclr/marshal.h>
#include "Pool.h"
#include "JudgeException.h"

using namespace System;
using namespace msclr::interop;

namespace Judge {

Pool::Pool(String ^tempDir)
	: pool_(nullptr)
{
	marshal_context mc;
	struct judge_pool *result;

	jstatus_t status = judge_create_pool(&result,
		mc.marshal_as<const char *>(tempDir));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}

	pool_ = result;
}

Pool::!Pool()
{
	if (pool_) {
		judge_destroy_pool(pool_);
		pool_ = nullptr;
	}
}

struct judge_pool *Pool::Handle()
{
	if (!pool_) {
		throw gcnew ObjectDisposedException("Pool");
	}
	return pool_;
}

void Pool::AddEnv(String ^username, String ^password)
{
	marshal_context mc;
	jstatus_t status = judge_add_env(pool_,
		mc.marshal_as<const char *>(username),
		mc.marshal_as<const char *>(password));
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
}

void Pool::AddEnvUnsafe()
{
	jstatus_t status = judge_add_env_unsafe(pool_);
	if (!JSUCCESS(status)) {
		throw gcnew JudgeException(status);
	}
}

}
