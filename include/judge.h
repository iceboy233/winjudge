#ifndef _JUDGE_H
#define _JUDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "misc.h"

struct judge_limit {
	uint32_t time_limit_ms;
	uint32_t memory_limit_kb;
	uint32_t active_process_limit;
	uint32_t stderr_output_limit;
};

struct judge_result {
	judge_flag_t flag;
	uint32_t time_usage_ms;
	uint32_t memory_usage_kb;
	runtime_error_t runtime_error;
	const char *judge_output;
	const char *user_output;
};

extern
jstatus_t
__stdcall
judge_create_pool(
	/* out */ struct judge_pool **pool,
	const char *temp_dir);

extern
void
__stdcall
judge_destroy_pool(
	struct judge_pool *pool);

extern
jstatus_t
__stdcall
judge_add_env(
	struct judge_pool *pool,
	const char *username,
	const char *password);

extern
jstatus_t
__stdcall
judge_add_env_unsafe(
	struct judge_pool *pool);

extern
jstatus_t
__stdcall
judge_create_compiler(
	/* out */ struct judge_compiler **compiler,

	// Compiler settings
	const char *executable_path,
	const char *command_line,
	const char *source_filename,
	const char *target_filename,
	/* optional */ struct judge_limit *compiler_limit,

	// Target settings
	const char *target_executable_path,
	const char *target_command_line);

extern
void
__stdcall
judge_destroy_compiler(
	struct judge_compiler *compiler);

extern
jstatus_t
__stdcall
judge_create_test(
	/* out */ struct judge_test **test,
	struct judge_pool *pool,
	struct judge_compiler *compiler,
	struct judgefs *source_fs,
	const char *source_path);

extern
void
__stdcall
judge_destroy_test(
	struct judge_test *test);

extern
jstatus_t
__stdcall
judge_add_testcase(
	struct judge_test *test,
	struct judgefs *data_fs,
	const char *input_path,
	const char *output_path,
	/* optional */ struct judge_limit *limit);

extern
jstatus_t
__stdcall
judge_add_testcase_spj(
	struct judge_test *test,
	struct judgefs *data_fs,
	const char *spj_prefix,
	const char *spj_path_rel,
	const char *spj_param,
	/* optional */ struct judge_limit *limit,
	/* optional */ struct judge_limit *spj_limit);

#define JUDGE_PHASE_NO_MORE (0)
#define JUDGE_PHASE_COMPILE (1)
#define JUDGE_PHASE_TESTCASE (2)
#define JUDGE_PHASE_SUMMARY (3)

extern
jstatus_t
__stdcall
judge_step_test(
	struct judge_test *test,
	/* out */ uint32_t *phase,
	/* out */ uint32_t *index,
	/* out */ const judge_result **result);

extern
void
__stdcall
judge_rundown_system(
	void);

#ifdef __cplusplus
}
#endif

#endif
