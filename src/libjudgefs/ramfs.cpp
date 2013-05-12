#include <string>
#include <map>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <judgefs.h>

using namespace std;

namespace {

class ramfs : public judgefs {
public:
	void *operator new(size_t size)
	{
		struct judgefs *result;
		jstatus_t status = judgefs_alloc(&result, size, &ramfs::vtable);
		if (!JSUCCESS(status)) {
			throw bad_alloc();
		}
		return result;
	}

	void operator delete(void *p)
	{
		judgefs_free(static_cast<struct judgefs *>(p));
	}

public:
	void set(const string &name, const string &value)
	{
		store_[name] = value;
	}

private:
	map<string, string> store_;
	typedef map<string, string>::iterator iter_t;

private:
	static jstatus_t __stdcall _open_(
		struct judgefs *fs,
		/* out */ struct judgefs_file **file,
		const char *path)
	{
		jstatus_t status = static_cast<ramfs *>(fs)->_open(file, path);
		if (JSUCCESS(status)) {
			judgefs_add_ref(fs);
		}
		return status;
	}

	static void __stdcall _close_(
		struct judgefs *fs,
		struct judgefs_file *file)
	{
		static_cast<ramfs *>(fs)->_close(file);
		judgefs_release(fs);
	}
	
	static jstatus_t __stdcall _read_(
		struct judgefs *fs,
		struct judgefs_file *file,
		char *buffer,
		uint32_t size,
		/* out */ uint32_t *actual_size)
	{
		return static_cast<ramfs *>(fs)->_read(file, buffer, size, actual_size);
	}

	static jstatus_t __stdcall _find_(
		struct judgefs *fs,
		/* out */ const char **path,
		const char *prefix,
		/* in out */ void **context)
	{
		return static_cast<ramfs *>(fs)->_find(path, prefix, context);
	}

	static void __stdcall _rundown_(
		struct judgefs *fs)
	{
		static_cast<ramfs *>(fs)->~ramfs();
	}

	static const struct judgefs_vtable vtable;

private:
	struct file_context {
		const char *buffer;
		uint32_t remain;
	};

private:
	jstatus_t _open(struct judgefs_file **file, const char *path)
	{
		iter_t iter = store_.find(path);
		if (iter == store_.end())
			return JSTATUS_NOT_FOUND;
		file_context *ctx = new file_context;
		ctx->buffer = iter->second.c_str();
		ctx->remain = iter->second.size();
		*file = reinterpret_cast<struct judgefs_file *>(ctx);
		return JSTATUS_SUCCESS;
	}

	void _close(struct judgefs_file *file)
	{
		delete reinterpret_cast<file_context *>(file);
	}

	jstatus_t _read(struct judgefs_file *file, char *buffer, uint32_t size, uint32_t *actual_size)
	{
		file_context *ctx = reinterpret_cast<file_context *>(file);
		size = min(size, static_cast<uint32_t>(ctx->remain));
		memcpy(buffer, ctx->buffer, size);
		ctx->buffer += size;
		ctx->remain -= size;
		*actual_size = size;
		return JSTATUS_SUCCESS;
	}

	jstatus_t _find(const char **path, const char *prefix, void **context)
	{
		if (*context == nullptr) {
			for (iter_t iter = store_.begin(); iter != store_.end(); ++iter) {
				if (strncmp(iter->first.c_str(), prefix, strlen(prefix)) == 0) {
					*path = iter->first.c_str();
					*context = reinterpret_cast<void *>(new iter_t(iter));
					return JSTATUS_SUCCESS;
				}
			}
			*path = nullptr;
		} else {
			iter_t &iter = *reinterpret_cast<iter_t *>(*context);
			++iter;
			if (iter == store_.end() || strncmp(iter->first.c_str(), prefix, strlen(prefix)) != 0) {
				delete &iter;
				*path = nullptr;
				*context = nullptr;
			} else {
				*path = iter->first.c_str();
			}
		}
		return JSTATUS_SUCCESS;
	}
};

}

const struct judgefs_vtable ramfs::vtable = {
	ramfs::_open_,
	ramfs::_close_,
	ramfs::_read_,
	ramfs::_find_,
	ramfs::_rundown_,
};

jstatus_t
__stdcall
ramfs_create(
	/* out */ struct judgefs **fs)
{
	try {
		*fs = new ramfs;
		return JSTATUS_SUCCESS;
	} catch (const bad_alloc &) {
		return JSTATUS_BAD_ALLOC;
	} catch (...) {
		return JSTATUS_GENERIC_ERROR;
	}
}

jstatus_t
__stdcall
ramfs_set(
	struct judgefs *fs,
	const char *name,
	const char *value)
{
	try {
		static_cast<ramfs *>(fs)->set(name, value);
		return JSTATUS_SUCCESS;
	} catch (const bad_alloc &) {
		return JSTATUS_BAD_ALLOC;
	} catch (...) {
		return JSTATUS_GENERIC_ERROR;
	}
}

jstatus_t
__stdcall
ramfs_set_n(
	struct judgefs *fs,
	const char *name,
	const char *value,
	size_t length)
{
	try {
		static_cast<ramfs *>(fs)->set(name, string(value, value + length));
		return JSTATUS_SUCCESS;
	} catch (const bad_alloc &) {
		return JSTATUS_BAD_ALLOC;
	} catch (...) {
		return JSTATUS_GENERIC_ERROR;
	}
}
