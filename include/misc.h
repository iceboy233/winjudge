#ifndef _MISC_H
#define _MISC_H

// Users should include `judge.h', not this file.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Type definition goes here.
typedef int32_t jstatus_t;
typedef int32_t judge_flag_t;
typedef int32_t runtime_error_t;

// Macros that help dealing with type `jstatus_t'.
#define JSUCCESS(x) ((x) >= 0)

// Add jstatus definitions here.
// Use non-negative value to indicate success, and negative value to indicate
// failure.
// All definitions starts with prefix `JSTATUS_'
#define JSTATUS_SUCCESS (0)
#define JSTATUS_GENERIC_ERROR (-1)
#define JSTATUS_NOT_IMPLEMENTED (-2)
#define JSTATUS_BAD_ALLOC (-3)
#define JSTATUS_CONCURRENCY_LIMIT_EXCEEDED (-4)
#define JSTATUS_NOT_FOUND (-5)
#define JSTATUS_LOGON_FAILURE (-6)
#define JSTATUS_PRIVILEGE_NOT_HELD (-7)
#define JSTATUS_PARAMETER_TOO_LARGE (-8)
#define JSTATUS_NOT_SUPPORTED (-9)
#define JSTATUS_INCORRECT_HANDLE (-10)

// Judge flags definitions go here.
#define JUDGE_NONE (0)
#define JUDGE_ACCEPTED (1)
#define JUDGE_WRONG_ANSWER (2)
#define JUDGE_RUNTIME_ERROR (3)
#define JUDGE_TIME_LIMIT_EXCEEDED (4)
#define JUDGE_MEMORY_LIMIT_EXCEEDED (5)
#define JUDGE_COMPILE_ERROR (6)

// Runtime error details go here.
#define RUNTIME_UNKNOWN (0)
#define RUNTIME_START_FAILED (1)
#define RUNTIME_READ_ACCESS_VIOLATION (2)
#define RUNTIME_WRITE_ACCESS_VIOLATION (3)
#define RUNTIME_EXECUTE_ACCESS_VIOLATION (4)
#define RUNTIME_DIVIDE_BY_ZERO (5)
#define RUNTIME_STACK_OVERFLOW (6)

#ifdef __cplusplus
}
#endif

#endif
