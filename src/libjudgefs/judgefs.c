#include <stddef.h>
#include <stdlib.h>
#include <judgefs.h>
#include <windows.h>

jstatus_t
__stdcall
judgefs_alloc(
	/* out */ struct judgefs **fs,
	size_t size,
	const struct judgefs_vtable *vtable)
{
	struct judgefs *result = (struct judgefs *)malloc(size);
	if (!result) {
		return JSTATUS_BAD_ALLOC;
	}

	result->vtable = vtable;
	result->ref_count = 1;
	*fs = result;
	return JSTATUS_SUCCESS;
}

void
__stdcall
judgefs_free(
	struct judgefs *fs)
{
	free(fs);
}

void
__stdcall
judgefs_add_ref(
	struct judgefs *fs)
{
	InterlockedIncrement(&fs->ref_count);
}

void
__stdcall
judgefs_release(
	struct judgefs *fs)
{
	if (!InterlockedDecrement(&fs->ref_count)) {
		fs->vtable->rundown(fs);
		judgefs_free(fs);
	}
}
