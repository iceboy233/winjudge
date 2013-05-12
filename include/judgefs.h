#ifndef _JUDGEFS_H
#define _JUDGEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "misc.h"

// judgefs
struct judgefs;
struct judgefs_file;

struct judgefs_vtable {
	jstatus_t (__stdcall *open)(
		struct judgefs *fs,
		/* out */ struct judgefs_file **file,
		const char *path);

	void (__stdcall *close)(
		struct judgefs *fs,
		struct judgefs_file *file);
	
	jstatus_t (__stdcall *read)(
		struct judgefs *fs,
		struct judgefs_file *file,
		char *buffer,
		uint32_t size,
		/* out */ uint32_t *actual_size);

	jstatus_t (__stdcall *find)(
		struct judgefs *fs,
		/* out */ const char **path,
		const char *prefix,
		/* in out */ void **context);

	void (__stdcall *rundown)(
		struct judgefs *fs);

};

struct judgefs {
	const struct judgefs_vtable *vtable;
	volatile uint32_t ref_count;
};

jstatus_t
__stdcall
judgefs_alloc(
	/* out */ struct judgefs **fs,
	size_t size,
	const struct judgefs_vtable *vtable);

extern
void
__stdcall
judgefs_free(
	struct judgefs *fs);

extern
void
__stdcall
judgefs_add_ref(
	struct judgefs *fs);

extern
void
__stdcall
judgefs_release(
	struct judgefs *fs);

// ramfs
extern
jstatus_t
__stdcall
ramfs_create(
	/* out */ struct judgefs **fs);

extern
jstatus_t
__stdcall
ramfs_set(
	struct judgefs *fs,
	const char *name,
	const char *value);

extern
jstatus_t
__stdcall
ramfs_set_n(
	struct judgefs *fs,
	const char *name,
	const char *value,
	size_t length);

// zipfs
extern
jstatus_t
__stdcall
zipfs_create(
	/* out */ struct judgefs **fs,
	const char *path);

#ifdef __cplusplus
}
#endif

#endif
