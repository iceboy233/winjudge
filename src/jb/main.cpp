#include <cstring>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <exception>
#include <stlsoft/synch/lock_scope.hpp>
#include <winstl/synch/thread_mutex.hpp>
#include <windows.h>
#include <process.h>
#include <judge.h>
#include <judgefs.h>
#include "../libjudge/fs_filebuf.hpp"
#include "../libjudge/exception.hpp"

using namespace std;
using namespace judge;
using stlsoft::lock_scope;
using winstl::thread_mutex;

namespace {

template <std::size_t buffer_size>
bool stream_copy(std::istream &in, std::ostream &out)
{
	char buffer[buffer_size];

	while (!in.eof()) {
		in.read(buffer, sizeof(buffer));
		if (in.bad()) return false;
		out.write(buffer, in.gcount());
		if (out.fail()) return false;
	}

	return true;
}

int str_to_int(const string &s)
{
	istringstream ss(s);
	int i;
	if (!(ss >> i))
		throw runtime_error("parse error");
	return i;
}

vector<string> split(const string &s, char delim)
{
	vector<string> result;
	string t;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i) {
		if (*i == delim) {
			result.push_back(move(t));
			t.clear();
		} else {
			t.push_back(*i);
		}
	}
	result.push_back(move(t));
	return result;
}

bool help = false;
int verbose = 0;
const char *package = nullptr;
const char *source = nullptr;
const char *compiler = nullptr;
bool loop = false;
judge_flag_t flag = JUDGE_ACCEPTED;
const char *env = nullptr;
const char *thread = nullptr;
bool benchmark = false;
struct judge_pool *pool;
struct judge_compiler *compiler_object;
thread_mutex mutex;
struct judgefs *ramfs;
struct judgefs *zipfs;
DWORD start_tick;
int test_finished = 0;

void print_help()
{
	cout << "Usage: jb [options] file" << endl;
	cout << "Options:" << endl;
	cout << "  -h[elp]\t\tPrint help" << endl;
	cout << "  -v[erbose]\t\tMore verbose" << endl;
	cout << "  -p[ackage] file\tSpecify package file" << endl;
	cout << "  -c[ompiler] setting\tSpecify compiler setting" << endl;
	cout << "\t\t\t\"exe;cmdline;input;output;target_exe;target_cmdline\"" << endl;
	cout << "  -l[oop]\t\tInfinite loop" << endl;
	cout << "  -e[nv] n\t\tSpecify number of environments" << endl;
	cout << "  -t[hread] n\t\tSpecify number of threads" << endl;
	cout << "  -b[enchmark]\t\tCollect performance information" << endl;
	cout << endl;
	cout << "For bug reporting instructions, please see" << endl;
	cout << "(http://code.google.com/p/winjudge/issues/list)." << endl;
}

string temp_path()
{
	DWORD result = ::GetTempPathA(0, nullptr);
	if (result == 0)
		throw runtime_error("could not get system temp path");
	vector<char> tempPath(result);
	result = ::GetTempPathA(static_cast<DWORD>(tempPath.size()), &*tempPath.begin());
	if ((result == 0) || (result > tempPath.size()))
		throw runtime_error("Could not get system temp path");
	return string(tempPath.begin(), tempPath.begin() + static_cast<std::size_t>(result));
}

string flag_to_str(judge_flag_t flag)
{
	switch (flag) {
	case JUDGE_ACCEPTED:
		return "Accepted";
	case JUDGE_WRONG_ANSWER:
		return "Wrong Answer";
	case JUDGE_RUNTIME_ERROR:
		return "Runtime Error";
	case JUDGE_TIME_LIMIT_EXCEEDED:
		return "Time Limit Exceeded";
	case JUDGE_MEMORY_LIMIT_EXCEEDED:
		return "Memory Limit Exceeded";
	case JUDGE_COMPILE_ERROR:
		return "Compile Error";
	default:
		return "None";
	}
}

unsigned int __stdcall thread_entry(void *param)
{
	intptr_t thread_id = reinterpret_cast<intptr_t>(param);
	jstatus_t status;

	try {
		if (verbose >= 3) {
			lock_scope<thread_mutex> lock(mutex);
			cout << "[" << thread_id << "] thread started" << endl;
		}

		do {
			struct judge_test *test;
			status = judge_create_test(&test, pool, compiler_object, ramfs, "source");
			if (!JSUCCESS(status))
				throw judge_exception(status);

			{
				if (verbose >= 3) {
					lock_scope<thread_mutex> lock(mutex);
					cout << "[" << thread_id << "] Debug: reading config" << endl;
				}

				fs_filebuf fb(zipfs, "config.ini");
				istream is(&fb);
				string s;
				do {
					if (!getline(is, s))
						throw runtime_error("failed to read config");
				} while (s.empty());
				int entries = str_to_int(s);

				if (verbose >= 3) {
					lock_scope<thread_mutex> lock(mutex);
					cout << "[" << thread_id << "] Debug: config have " << entries << " entries" << endl;
				}

				for (int i = 0; i < entries; ++i) {
					do {
						if (!getline(is, s))
							throw runtime_error("failed to read config");
					} while (s.empty());
					
					vector<string> entry = split(s, '|');
					if (entry.size() < 4)
						throw runtime_error("failed to read config");
					
					judge_limit limit;
					limit.time_limit_ms = static_cast<uint32_t>(str_to_int(entry[2])) * 1000;
					limit.memory_limit_kb = 0;
					limit.active_process_limit = 1;
					limit.stderr_output_limit = 4096;
					status = judge_add_testcase(test, zipfs, ("Input/" + entry[0]).c_str(),
						("Output/" + entry[1]).c_str(), &limit);
					if (!JSUCCESS(status))
						throw judge_exception(status);
					
				}
			}

			uint32_t phase, index;
			const struct judge_result *result;

			while (true) {
				status = judge_step_test(test, &phase, &index, &result);
				if (!JSUCCESS(status))
					throw judge_exception(status);
				if (phase == JUDGE_PHASE_COMPILE) {
					if (verbose >= 1) {
						lock_scope<thread_mutex> lock(mutex);
						cout << "[" << thread_id << "] Compile: " << flag_to_str(result->flag) << endl;
						if (verbose >= 2) {
							cout << result->judge_output;
						}
					}
				} else if (phase == JUDGE_PHASE_TESTCASE) {
					if (verbose >= 1) {
						lock_scope<thread_mutex> lock(mutex);
						cout << "[" << thread_id << "] Testcase " << index + 1 << ": " << flag_to_str(result->flag) << " ("
							<< result->time_usage_ms << "ms / " << result->memory_usage_kb << " kb)" << endl;
					}
				} else if (phase == JUDGE_PHASE_SUMMARY) {
					lock_scope<thread_mutex> lock(mutex);
					cout << "[" << thread_id << "] " << flag_to_str(result->flag) << " ("
						<< result->time_usage_ms << "ms / " << result->memory_usage_kb << " kb)";
					if (benchmark) {
						cout << " - " << static_cast<double>(++test_finished) * 1000 / (::GetTickCount() - start_tick) << " tests per sec";
					}	
					cout << endl;
					flag = result->flag;
				} else {
					break;
				}
			}

			judge_destroy_test(test);
		} while (loop);
		return 0;

	} catch (const exception &ex) {
		lock_scope<thread_mutex> lock(mutex);
		cerr << "[" << thread_id << "] error: " << ex.what() << endl;
		return 1;
	}
}

}

int main(int argc, char *argv[])
{
	try {
		bool next_package = false;
		bool next_compiler = false;
		bool next_env = false;
		bool next_thread = false;

		for (int i = 1; i < argc; ++i) {
			if (next_package) {
				if (package != nullptr)
					throw runtime_error("package is already specified");
				package = argv[i];
				next_package = false;
			} else if (next_compiler) {
				if (compiler != nullptr)
					throw runtime_error("compiler is already specified");
				compiler = argv[i];
				next_compiler = false;
			} else if (next_env) {
				if (env != nullptr)
					throw runtime_error("env is already specified");
				env = argv[i];
				next_env = false;
			} else if (next_thread) {
				if (thread != nullptr)
					throw runtime_error("thread is already specified");
				thread = argv[i];
				next_thread = false;
			} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
				help = true;
			} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-verbose")) {
				++verbose;
			} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "-package")) {
				next_package = true;
			} else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "-compiler")) {
				next_compiler = true;
			} else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "-env")) {
				next_env = true;
			} else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "-thread")) {
				next_thread = true;
			} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "-loop")) {
				loop = true;
			} else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "-benchmark")) {
				benchmark = true;
			} else {
				if (source != nullptr)
					throw runtime_error("source is already specified");
				source = argv[i];
			}
		}

		if (help) {
			print_help();
			return 0;
		}

		if (package == nullptr)
			throw runtime_error("package is not specified, try -h for help");
		if (source == nullptr)
			throw runtime_error("source is not specified, try -h for help");
		if (compiler == nullptr)
			compiler = "";

		jstatus_t status = judge_create_pool(&pool, temp_path().c_str());
		if (!JSUCCESS(status)) {
			throw judge_exception(status);
		}

		int env_n = 1;
		if (env != nullptr) {
			if (sscanf(env, "%d", &env_n) != 1) {
				env_n = 1;
			}
		}
		for (int i = 0; i < env_n; ++i) {
			status = judge_add_env_unsafe(pool);
			if (!JSUCCESS(status))
				throw judge_exception(status);
		}

		vector<string> compiler_settings = split(compiler, ';');
		while (compiler_settings.size() < 6)
			compiler_settings.push_back(string());
		status = judge_create_compiler(&compiler_object,
			compiler_settings[0].c_str(), compiler_settings[1].c_str(),
			compiler_settings[2].c_str(), compiler_settings[3].c_str(),
			nullptr,
			compiler_settings[4].c_str(), compiler_settings[5].c_str());
		if (!JSUCCESS(status))
			throw judge_exception(status);

		int thread_n = 1;
		if (thread != nullptr) {
			if (sscanf(thread, "%d", &thread_n) != 1) {
				thread_n = 1;
			}
		}

		status = ramfs_create(&ramfs);
		if (!JSUCCESS(status))
			throw judge_exception(status);

		{
			ifstream source_in(source, ios::binary);
			ostringstream source_out;
			if (!source_in.is_open() || !stream_copy<4096>(source_in, source_out))
				throw runtime_error("failed to read source code");
			string str = source_out.str();
			status = ramfs_set_n(ramfs, "source", str.c_str(), str.size());
			if (!JSUCCESS(status))
				throw judge_exception(status);
		}

		status = zipfs_create(&zipfs, package);
		if (!JSUCCESS(status))
			throw judge_exception(status);

		start_tick = ::GetTickCount();
		queue<HANDLE> threads;
		for (int i = 0; i < thread_n; ++i) {
			uintptr_t result = _beginthreadex(nullptr, 0, thread_entry, reinterpret_cast<void *>(i), 0, nullptr);
			if (!result)
				throw runtime_error("failed to create thread");
			threads.push(reinterpret_cast<HANDLE>(result));
		}

		while (!threads.empty()) {
			::WaitForSingleObject(threads.front(), INFINITE);
			::CloseHandle(threads.front());
			threads.pop();
		}

	} catch (const exception &ex) {
		lock_scope<thread_mutex> lock(mutex);
		cerr << "error: " << ex.what() << endl;
		return 1;
	}

	if (flag == JUDGE_ACCEPTED) {
		return 0;
	} else {
		return 1000 + flag;
	}
}
