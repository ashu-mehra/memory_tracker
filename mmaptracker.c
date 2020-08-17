#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <libsyscall_intercept_hook_point.h>

static FILE *traceFile = NULL;

static int
hook(long syscall_number,
	long arg0, long arg1,
	long arg2, long arg3,
	long arg4, long arg5,
	long *result)
{
	if (syscall_number == SYS_mmap) {
		*result = syscall_no_intercept(syscall_number, arg0, arg1, arg2, arg3, arg4, arg5);
		if (NULL == traceFile) {
			fprintf(stdout, "mmap(syscall): %p 0x%zx\n", (void *)(*result), (size_t)arg1);
		} else {
			fprintf(traceFile, "mmap(syscall): %p 0x%zx\n", (void *)(*result), (size_t)arg1);
			fflush(traceFile);
		}
		return 0;
	} else if (syscall_number == SYS_munmap) {
		*result = syscall_no_intercept(syscall_number, arg0, arg1, arg2, arg3, arg4, arg5);
		if (NULL == traceFile) {
			fprintf(stdout, "munmap(syscall): %p 0x%zx\n", (void *)arg0, (size_t)arg1);
		} else {
			fprintf(traceFile, "munmap(syscall): %p 0x%zx\n", (void *)arg0, (size_t)arg1);
			fflush(traceFile);
		}
		return 0;

	} else {
		return -1;
	}
}

static __attribute__((constructor)) void
init(void)
{
	char *file = getenv("MMAP_TRACE");
	if (NULL != file) {
		traceFile = fopen(file, "w");
	}
	// Set up the callback function
	intercept_hook_point = hook;
}
