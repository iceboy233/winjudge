#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zip.h>
#include <judgefs.h>
#include <windows.h>

struct zipfs {
	struct judgefs judgefs;
	struct zip *zip;
	CRITICAL_SECTION zip_cs;
};

static jstatus_t zep_to_jstatus(int zep)
{
	switch (zep) {
	case ZIP_ER_OK:
		return JSTATUS_SUCCESS;
	case ZIP_ER_NOENT:
		return JSTATUS_NOT_FOUND;
	case ZIP_ER_MEMORY:
		return JSTATUS_BAD_ALLOC;
	default:
		return JSTATUS_GENERIC_ERROR;
	}
}

static jstatus_t __stdcall open(
	struct judgefs *fs,
	/* out */ struct judgefs_file **file,
	const char *path)
{
	struct zipfs *zipfs = (struct zipfs *)fs;
	struct zip_file *zip_file;
	int zep;
	
	EnterCriticalSection(&zipfs->zip_cs);
	zip_file = zip_fopen(zipfs->zip, path, ZIP_FL_NOCASE);
	
	if (zip_file) {
		LeaveCriticalSection(&zipfs->zip_cs);
		*file = (struct judgefs_file *)zip_file;
		judgefs_add_ref(fs);
		return JSTATUS_SUCCESS;
	} else {
		zip_error_get(zipfs->zip, &zep, NULL);
		LeaveCriticalSection(&zipfs->zip_cs);
		return zep_to_jstatus(zep);
	}
}

static void __stdcall close(
	struct judgefs *fs,
	struct judgefs_file *file)
{
	struct zipfs *zipfs = (struct zipfs *)fs;
	EnterCriticalSection(&zipfs->zip_cs);
	zip_fclose((struct zip_file *)file);
	LeaveCriticalSection(&zipfs->zip_cs);
	judgefs_release(fs);
}
	
static jstatus_t __stdcall read(
	struct judgefs *fs,
	struct judgefs_file *file,
	char *buffer,
	uint32_t size,
	/* out */ uint32_t *actual_size)
{
	struct zipfs *zipfs = (struct zipfs *)fs;
	struct zip_file *zip_file = (struct zip_file *)file;
	zip_int64_t result;
	int zep;

	EnterCriticalSection(&zipfs->zip_cs);
	result = zip_fread(zip_file, buffer, size);
	if (result >= 0) {
		LeaveCriticalSection(&zipfs->zip_cs);
		*actual_size = (uint32_t)result;
		return JSTATUS_SUCCESS;
	} else {
		zip_file_error_get(zip_file, &zep, NULL);
		LeaveCriticalSection(&zipfs->zip_cs);
		return zep_to_jstatus(zep);
	}
}

static jstatus_t __stdcall find(
	struct judgefs *fs,
	/* out */ const char **path,
	const char *prefix,
	/* in out */ void **context)
{
	struct zipfs *zipfs = (struct zipfs *)fs;
	uintptr_t *index = (uintptr_t *)context;
	zip_uint64_t num_entries;
	int zep;

	EnterCriticalSection(&zipfs->zip_cs);
	num_entries = zip_get_num_entries(zipfs->zip, ZIP_FL_NOCASE);

	while (1) {
		if (*index < num_entries) {
			const char *name = zip_get_name(zipfs->zip, *index, ZIP_FL_NOCASE);
			if (!name) {
				zip_error_get(zipfs->zip, &zep, NULL);
				LeaveCriticalSection(&zipfs->zip_cs);
				return zep_to_jstatus(zep);
			}
			++*index;
			if (strncmp(name, prefix, strlen(prefix)) == 0) {
				LeaveCriticalSection(&zipfs->zip_cs);
				*path = name;
				return JSTATUS_SUCCESS;
			}
		} else {
			LeaveCriticalSection(&zipfs->zip_cs);
			*path = NULL;
			*index = 0;
			return JSTATUS_SUCCESS;
		}
	}
}

static void __stdcall rundown(
	struct judgefs *fs)
{
	struct zipfs *zipfs = ((struct zipfs *)fs);
	zip_close(zipfs->zip);
	DeleteCriticalSection(&zipfs->zip_cs);
}

static struct judgefs_vtable vtable = {
	open, close, read, find, rundown,
};

jstatus_t
__stdcall
zipfs_create(
	/* out */ struct judgefs **fs,
	const char *path)
{
	struct judgefs *result;
	int zep;
	struct zipfs *zipfs;
	jstatus_t status;
	
	status = judgefs_alloc(&result, sizeof(struct zipfs), &vtable);
	if (!JSUCCESS(status)) {
		return JSTATUS_BAD_ALLOC;
	}

	zipfs = (struct zipfs *)result;
	zipfs->zip = zip_open(path, ZIP_FL_NOCASE, &zep);
	if (zipfs->zip) {
		InitializeCriticalSection(&zipfs->zip_cs);
		*fs = result;
		return JSTATUS_SUCCESS;
	} else {
		judgefs_free(result);
		return zep_to_jstatus(zep);
	}
}
