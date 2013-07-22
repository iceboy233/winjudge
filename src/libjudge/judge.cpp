#include <memory>
#include <cassert>
#include <winstl/filesystem/path.hpp>
#include <judge.h>
#include "pool.hpp"
#include "temp_dir.hpp"
#include "env.hpp"
#include "compiler.hpp"
#include "test.hpp"
#include "testcase.hpp"
#include "util.hpp"

using namespace std;
using namespace judge;
using winstl::path_a;

namespace {

judge_limit sanitize(judge_limit *limit)
{
	judge_limit result = {0};
	if (limit) {
		result = *limit;
	}
	return result;
}

}

jstatus_t
__stdcall
judge_create_pool(
	/* out */ struct judge_pool **pool,
	const char *temp_dir)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::pool> p(new judge::pool);
		shared_ptr<judge::temp_dir> td(new judge::temp_dir(p, path_a(temp_dir)));
		p->set_temp_dir(td);
		*pool = reinterpret_cast<struct judge_pool *>(
			new shared_ptr<judge::pool>(move(p)));
		return JSTATUS_SUCCESS;
	});
}

void
__stdcall
judge_destroy_pool(
	struct judge_pool *pool)
{
	delete reinterpret_cast<shared_ptr<judge::pool> *>(pool);
}

jstatus_t
__stdcall
judge_add_env(
	struct judge_pool *pool,
	const char *username,
	const char *password)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::pool> &p(*reinterpret_cast<shared_ptr<judge::pool> *>(pool));
		std::unique_ptr<judge::env> env(new judge::restricted_env(p, username, password));
		p->add_env(move(env));
		return JSTATUS_SUCCESS;
	});
}

jstatus_t
__stdcall
judge_add_env_unsafe(
	struct judge_pool *pool)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::pool> &p = *reinterpret_cast<shared_ptr<judge::pool> *>(pool);
		std::unique_ptr<judge::env> env(new judge::env(p));
		p->add_env(move(env));
		return JSTATUS_SUCCESS;
	});
}

jstatus_t
__stdcall
judge_create_compiler(
	/* out */ struct judge_compiler **compiler,
	const char *executable_path,
	const char *command_line,
	const char *source_filename,
	const char *target_filename,
	struct judge_limit *compiler_limit,
	const char *target_executable_path,
	const char *target_command_line)
{
	// allow omission for source and target filename
	if (*source_filename == '\0' && *target_filename == '\0') {
		source_filename = target_filename = "tmpfile";
	}

	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::compiler> p(
			new judge::compiler(executable_path, command_line,
				source_filename, target_filename, sanitize(compiler_limit),
				target_executable_path, target_command_line));
		*compiler = reinterpret_cast<struct judge_compiler *>(
			new shared_ptr<judge::compiler>(move(p)));
		return JSTATUS_SUCCESS;
	});
}

void
__stdcall
judge_destroy_compiler(
	struct judge_compiler *compiler)
{
	delete reinterpret_cast<shared_ptr<judge::compiler> *>(compiler);
}

jstatus_t
__stdcall
judge_create_test(
	/* out */ struct judge_test **test,
	struct judge_pool *pool,
	struct judge_compiler *compiler,
	struct judgefs *source_fs,
	const char *source_path)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::compiler> &c = *reinterpret_cast<shared_ptr<judge::compiler> *>(compiler);
		shared_ptr<judge::pool> &p(*reinterpret_cast<shared_ptr<judge::pool> *>(pool));
		shared_ptr<judge::test> t(
			new judge::test(p, c, source_fs, source_path));
		*test = reinterpret_cast<struct judge_test *>(
			new shared_ptr<judge::test>(move(t)));
		return JSTATUS_SUCCESS;
	});
}

void
__stdcall
judge_destroy_test(
	struct judge_test *test)
{
	delete reinterpret_cast<shared_ptr<judge::test> *>(test);
}

jstatus_t
__stdcall
judge_add_testcase(
	struct judge_test *test,
	struct judgefs *data_fs,
	const char *input_path,
	const char *output_path,
	struct judge_limit *limit)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::test> &t(*reinterpret_cast<shared_ptr<judge::test> *>(test));
		shared_ptr<judge::testcase> tc(
			new judge::testcase_impl(data_fs, input_path, output_path, sanitize(limit)));
		t->add(tc);
		return JSTATUS_SUCCESS;
	});
}

jstatus_t
__stdcall
judge_add_testcase_spj(
	struct judge_test *test,
	struct judgefs *data_fs,
	const char *spj_prefix,
	const char *spj_path_rel,
	const char *spj_param,
	struct judge_limit *limit,
	struct judge_limit *spj_limit)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::test> &t(*reinterpret_cast<shared_ptr<judge::test> *>(test));
		shared_ptr<judge::testcase> tc(
			new judge::testcase_spj_impl(data_fs, spj_prefix, spj_path_rel, spj_param,
			sanitize(limit), sanitize(spj_limit)));
		t->add(tc);
		return JSTATUS_SUCCESS;
	});
}

jstatus_t
__stdcall
judge_step_test(
	struct judge_test *test,
	/* out */ uint32_t *phase,
	/* out */ uint32_t *index,
	/* out */ const judge_result **result)
{
	return util::wrap([=]()->jstatus_t {
		shared_ptr<judge::test> &t(*reinterpret_cast<shared_ptr<judge::test> *>(test));
		t->step();
		*phase = t->phase();
		*index = t->index();
		*result = &t->last_result();
		return JSTATUS_SUCCESS;
	});
}

void
__stdcall
judge_rundown_system(
	void)
{
}
