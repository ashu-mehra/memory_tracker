#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <mcheck.h>

#define CALLOC_BUFFER_SIZE (64*1024)

static pthread_key_t key;
static int my_init_done = 0;
static FILE *traceFile = NULL;
static char calloc_buffer[CALLOC_BUFFER_SIZE];
static char *current = calloc_buffer;

void __attribute__ ((constructor)) my_init(void);

void *(*real_malloc)(size_t) = NULL;
#if 1 
void *(*real_calloc)(size_t, size_t) = NULL;
void *(*real_realloc)(void *, size_t) = NULL;
void *(*real_memalign)(size_t, size_t) = NULL;
void (*real_free)(void *) = NULL;
#endif

void *(*real_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
void *(*real_mmap64)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int (*real_munmap)(void *addr, size_t length);

void my_init()
{
	char *file = NULL;
#if 1 
	real_calloc = dlsym(RTLD_NEXT, "calloc");
#endif
	real_malloc = dlsym(RTLD_NEXT, "malloc");
#if 1
	real_realloc = dlsym(RTLD_NEXT, "realloc");
	real_memalign = dlsym(RTLD_NEXT, "memalign");
	real_free = dlsym(RTLD_NEXT, "free");
#endif
	real_mmap = dlsym(RTLD_NEXT, "mmap");
	real_mmap64 = dlsym(RTLD_NEXT, "mmap64");
	real_munmap = dlsym(RTLD_NEXT, "munmap");
	//printf("real_malloc: %p\nreal_free: %p\nreal_mmap: %p\nreal_mmap64: %p\nreal_munmap: %p\n", real_malloc, real_free, real_mmap, real_mmap64, real_munmap);
	file = getenv("ALLOCATION_TRACE");
	if (NULL != file) {
		traceFile = fopen(file, "w");
	}
	pthread_key_create(&key, NULL);
	my_init_done = 1;
	//mtrace();
}

#if 1 

void *malloc(size_t size)
{
	if (NULL == real_malloc) {
		real_malloc = dlsym(RTLD_NEXT, "malloc");
	}
	void *addr = real_malloc(size);
	if (!my_init_done) return addr;
	
	int *enable_tracing = pthread_getspecific(key);
	if (enable_tracing == NULL) {
		enable_tracing = real_malloc(sizeof(int));
		*enable_tracing = 1;
		pthread_setspecific(key, enable_tracing);
	}
	if ((NULL != enable_tracing) && (*enable_tracing == 1)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(0);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) {
			// format: malloc: <library_name> <function_name> <start> <end>
			if (NULL != traceFile) {
				fprintf(traceFile, "malloc: %p 0x%zx %s %s %p\n", addr, size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				fflush(traceFile);
			} else {
				fprintf(stderr, "malloc: %p 0x%zx %s %s %p\n", addr, size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
			}
		}
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
	return addr;
}

void *calloc(size_t nmemb, size_t size)
{
	void *addr = NULL;
	if (NULL == real_calloc) {
		if ((current + nmemb*size) < calloc_buffer + CALLOC_BUFFER_SIZE) {
			char *ptr = current;
			current += (nmemb*size);
			return (void *)ptr;
		} else {
			printf("buffer for calloc is too small!\n");
			exit(-1);
		}
	} else {
		addr = real_calloc(nmemb, size);
	}

	if (!my_init_done) return addr;
	
	int *enable_tracing = pthread_getspecific(key);
	if (enable_tracing == NULL) {
		enable_tracing = real_malloc(sizeof(int));
		*enable_tracing = 1;
		pthread_setspecific(key, enable_tracing);
	}
	if ((NULL != enable_tracing) && (*enable_tracing == 1)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(0);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) {
			// format: malloc: <library_name> <function_name> <start> <end>
			if (NULL != traceFile) {
				fprintf(traceFile, "calloc: %p 0x%zx %s %s %p\n", addr, nmemb*size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				fflush(traceFile);
			} else {
				fprintf(stderr, "calloc: %p 0x%zx %s %s %p\n", addr, nmemb*size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
			}
		}
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
	return addr;
}

void *realloc(void *ptr, size_t size)
{
	if (NULL == real_realloc) {
		real_realloc = dlsym(RTLD_NEXT, "realloc");
	}
	void *addr = real_realloc(ptr, size);
	if (!my_init_done) return addr;
	
	int *enable_tracing = pthread_getspecific(key);
	if (enable_tracing == NULL) {
		enable_tracing = real_malloc(sizeof(int));
		*enable_tracing = 1;
		pthread_setspecific(key, enable_tracing);
	}
	if ((NULL != enable_tracing) && (*enable_tracing == 1)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(0);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) {
			// format: malloc: <library_name> <function_name> <start> <end>
			if (NULL != traceFile) {
				if (NULL != ptr) {
					fprintf(traceFile, "free(realloc): %p %s %s %p\n", ptr, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				}
				if (0 != size) {
					fprintf(traceFile, "malloc(realloc): %p 0x%zx %s %s %p\n", addr, size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				}
				fflush(traceFile);
			} else {
				if (NULL != ptr) {
					fprintf(stderr, "free(realloc): %p %s %s %p\n", ptr, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				}
				if (0 != size) {
					fprintf(stderr, "malloc(realloc): %p 0x%zx %s %s %p\n", addr, size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				}
			}
		}
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
	return addr;
}

void *memalign(size_t alignment, size_t size)
{
	if (NULL == real_memalign) {
		real_memalign = dlsym(RTLD_NEXT, "memalign");
	}
	void *addr = real_memalign(alignment, size);
	if (!my_init_done) return addr;
	
	int *enable_tracing = pthread_getspecific(key);
	if (enable_tracing == NULL) {
		enable_tracing = real_malloc(sizeof(int));
		*enable_tracing = 1;
		pthread_setspecific(key, enable_tracing);
	}
	if ((NULL != enable_tracing) && (*enable_tracing == 1)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(0);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) {
			// format: malloc: <library_name> <function_name> <start> <end>
			if (NULL != traceFile) {
				fprintf(traceFile, "memalign: %p 0x%zx %s %s %p\n", addr, size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				fflush(traceFile);
			} else {
				fprintf(stderr, "memalign: %p 0x%zx %s %s %p\n", addr, size, dlinfo.dli_fname, dlinfo.dli_sname, caller);
			}
		}
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
	return addr;
}

void free(void *ptr)
{
	char *cptr = (char *)ptr;
	if ((cptr >= calloc_buffer) && (cptr < (calloc_buffer + CALLOC_BUFFER_SIZE))) {
		return;
	}
	if (NULL == real_free) {
		real_free = dlsym(RTLD_NEXT, "free");
	}
	real_free(ptr);
	if (!my_init_done) return;
		
	int *enable_tracing = pthread_getspecific(key);
	if ((NULL != enable_tracing) && (*enable_tracing == 1) && (NULL != ptr)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(0);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) { 
			// format: malloc: <library_name> <function_name> <start> <end>
			if (NULL != traceFile) {
				fprintf(traceFile, "free: %p %s %s %p\n", ptr, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				fflush(traceFile);
			} else {
				fprintf(stderr, "free: %p %s %s %p\n", ptr, dlinfo.dli_fname, dlinfo.dli_sname, caller);
			}
		}	
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
	return;
}
#endif

void trace_mmap(void *addr, size_t length, const char *func)
{
	int *enable_tracing = pthread_getspecific(key);
	if (enable_tracing == NULL) {
		enable_tracing = real_malloc(sizeof(int));
		*enable_tracing = 1;
		pthread_setspecific(key, enable_tracing);
	}
	if ((NULL != enable_tracing) && (*enable_tracing == 1)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(1);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) {
			// format: malloc: <library_name> <function_name> <start> <end>
			if (NULL != traceFile) {
				fprintf(traceFile, "%s: %p 0x%zx %s %s %p\n", func, addr, length, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				fflush(traceFile);
			} else {
				fprintf(stderr, "%s: %p 0x%zx %s %s %p\n", func, addr, length, dlinfo.dli_fname, dlinfo.dli_sname, caller);
			}
		}
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
}

void *mmap64(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	if (NULL == real_mmap64) {
		real_mmap64 = dlsym(RTLD_NEXT, "mmap64");
	}
	void *ptr = real_mmap64(addr, length, prot, flags, fd, offset);
	if (!my_init_done) return ptr;
	
	trace_mmap(ptr, length, "mmap64");
	return ptr;
}	

void *__mmap64(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	if (NULL == real_mmap64) {
		real_mmap64 = dlsym(RTLD_NEXT, "mmap64");
	}
	void *ptr = real_mmap64(addr, length, prot, flags, fd, offset);
	if (!my_init_done) return ptr;
	
	trace_mmap(ptr, length, "__mmap64");
	return ptr;
}	

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	if (NULL == real_mmap) {
		real_mmap = dlsym(RTLD_NEXT, "mmap");
	}
	void *ptr = real_mmap(addr, length, prot, flags, fd, offset);
	if (!my_init_done) return ptr;

	trace_mmap(ptr, length, "mmap");
	return ptr;
}

void *__mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	if (NULL == real_mmap) {
		real_mmap = dlsym(RTLD_NEXT, "mmap");
	}
	void *ptr = real_mmap(addr, length, prot, flags, fd, offset);
	if (!my_init_done) return ptr;

	trace_mmap(ptr, length, "__mmap");
	return ptr;
}

void trace_munmap(void *addr, size_t length, const char *func)
{
	int *enable_tracing = pthread_getspecific(key);
	if ((NULL != enable_tracing) && (*enable_tracing == 1) && (NULL != addr)) {
		Dl_info dlinfo =  {0};
		void *caller = NULL;
		int rc = 0;

		*enable_tracing = 0;
		caller = __builtin_return_address(1);
		rc = dladdr(caller, &dlinfo);
		if (0 != rc) { 
			if (NULL != traceFile) {
				fprintf(traceFile, "%s: %p 0x%zx %s %s %p\n", func, addr, length, dlinfo.dli_fname, dlinfo.dli_sname, caller);
				fflush(traceFile);
			} else {
				fprintf(stderr, "%s: %p 0x%zx %s %s %p\n", func, addr, length, dlinfo.dli_fname, dlinfo.dli_sname, caller);
			}
		}	
	}
	if (NULL != enable_tracing) {
		*enable_tracing = 1;
	}
}

int munmap(void *addr, size_t length)
{
	int rc = 0;
	if (NULL == real_munmap) {
		real_munmap = dlsym(RTLD_NEXT, "munmap");
	}
	rc = real_munmap(addr, length);
	if (!my_init_done) return rc;
	
	trace_munmap(addr, length, "munmap");	
	return rc;
}

int __munmap(void *addr, size_t length)
{
	int rc = 0;
	if (NULL == real_munmap) {
		real_munmap = dlsym(RTLD_NEXT, "munmap");
	}
	rc = real_munmap(addr, length);
	if (!my_init_done) return rc;
	
	trace_munmap(addr, length, "__munmap");	
	return rc;
}
