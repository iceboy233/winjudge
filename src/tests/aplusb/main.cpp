#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>
#include <judge.h>
#include <judgefs.h>

using namespace std;

namespace {

judgefs *ramfs;
judgefs *zipfs;

string int_to_str(int i)
{
	ostringstream out;
	out << i;
	return out.str();
}

void create_fs()
{
	jstatus_t status;

	status = ramfs_create(&ramfs);
	assert(JSUCCESS(status));
	status = ramfs_set(ramfs, "source",
		"#include <stdio.h>\n"
		"\n"
		"int main(void)\n"
		"{\n"
		"	int a, b;\n"
		"	scanf(\"%d%d\", &a, &b);\n"
		"	printf(\"%d\\n\", a + b);\n"
		"}\n");
	assert(JSUCCESS(status));

	status = zipfs_create(&zipfs, "1000.zip");
	assert(JSUCCESS(status));
}

}

void print(int x)
{
	printf("%d\n", x);
}

int main()
{
	jstatus_t status;
	judge_pool *pool;
	judge_compiler *compiler;
	judge_test *test;
	judge_limit limit;

	create_fs();

	status = judge_create_pool(&pool, "E:\\temp");
	assert(JSUCCESS(status));
	status = judge_add_env_unsafe(pool);
	assert(JSUCCESS(status));
	status = judge_add_env_unsafe(pool);
	assert(JSUCCESS(status));
	limit.active_process_limit = 0;
	limit.time_limit_ms = 15000;
	limit.memory_limit_kb = 0;
	limit.stderr_output_limit = 0;
	status = judge_create_compiler(&compiler, "C:\\MinGW32\\bin\\gcc.exe",
		"gcc -ofoo.exe foo.c", "foo.c", "foo.exe", &limit, "", "");
	assert(JSUCCESS(status));
	status = judge_create_test(&test, pool, compiler, ramfs, "source");
	assert(JSUCCESS(status));
	limit.active_process_limit = 1;
	limit.time_limit_ms = 1000;
	limit.memory_limit_kb = 65536;
	limit.stderr_output_limit = 4096;
	for (int i = 0; i < 10; ++i) {
		status = judge_add_testcase(test, zipfs,
			("Input/input" + int_to_str(i) + ".txt").c_str(),
			("Output/output" + int_to_str(i) + ".txt").c_str(), &limit);
		assert(JSUCCESS(status));
	}

	while (true) {
		const char *phase_name[] = { nullptr, "compile", "case", "summary" };
		const char *flag_name[] = { nullptr, "AC", "WA", "RE", "TLE", "MLE", "CE" };
		uint32_t phase, index;
		const judge_result *result;
		status = judge_step_test(test, &phase, &index, &result);
		assert(JSUCCESS(status));
		if (phase == JUDGE_PHASE_NO_MORE)
			break;
		printf("[%s %d] %s, time = %dms, mem = %dkb\n",
			phase_name[phase], (int)index, flag_name[result->flag],
			(int)result->time_usage_ms, (int)result->memory_usage_kb);
	}

	judge_destroy_test(test);
	judge_destroy_compiler(compiler);
	judge_destroy_pool(pool);

	return 0;
}
