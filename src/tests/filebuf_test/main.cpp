#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>
#include <judgefs.h>
#include "os_filebuf.hpp"
#include "fs_filebuf.hpp"

using namespace std;
using namespace judge;

void write_os_test()
{
	HANDLE h = ::CreateFileA("d:\\test_1_to_10000.txt", GENERIC_WRITE,
		FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	os_filebuf fb(h);
	ostream os(&fb);
	for (int i = 1; i <= 100000; ++i) {
		os << i << '\n';
	}
}

void read_os_test()
{
	HANDLE h = ::CreateFileA("d:\\test_1_to_10000.txt", GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	os_filebuf fb(h);
	istream is(&fb);
	int i;
	long long sum = 0;
	while (is >> i)
		sum += i;
	cout << "sum = " << sum << endl;
}

void ram_fs_test()
{
	struct judgefs *fs;
	
	ramfs_create(&fs);

	// create files
	ramfs_set(fs, "hello", "hello world!");
	ramfs_set(fs, "foo/bar", "aaaaaaaa");
	ramfs_set(fs, "foo/baz", "bbbbbbbbbb");
	ramfs_set(fs, "foo/baq", "cccccccccccc");
	{
		ostringstream oss;
		for (int i = 1; i <= 100000; ++i)
			oss << i << '\n';
		ramfs_set(fs, "big_file", oss.str().c_str());
	}

	// find files
	const char *path;
	void *context = nullptr;

	while (JSUCCESS(fs->vtable.find(fs, &path, "foo/", &context)) && path) {
		cout << path << endl;
	}

	// read file
	struct judgefs_file *file;
	fs->vtable.open(fs, &file, "big_file");
	{
		fs_filebuf fb(fs, file);
		istream is(&fb);
		int i;
		long long sum = 0;
		while (is >> i)
			sum += i;
		cout << "sum = " << sum << endl;
	}

	fs->vtable.destroy(fs);
}

int main()
{
	write_os_test();
	read_os_test();
	ram_fs_test();
}
