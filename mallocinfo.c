#include <pthread.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void __attribute__ ((constructor)) my_init(void);

void *print_malloc_info(void *arg) {
	FILE *file = (FILE *)arg;
	int counter = 0;
	while (1) {
		struct mallinfo mi;
		mi = mallinfo();
		fprintf(file, "------------------- mallino (%d) -------------------\n", counter++);
		fprintf(file, "Total non-mmapped bytes (arena):       %d\n", mi.arena);
		fprintf(file, "# of free chunks (ordblks):            %d\n", mi.ordblks);
		fprintf(file, "# of free fastbin blocks (smblks):     %d\n", mi.smblks);
		fprintf(file, "# of mapped regions (hblks):           %d\n", mi.hblks);
		fprintf(file, "Bytes in mapped regions (hblkhd):      %d\n", mi.hblkhd);
		fprintf(file, "Max. total allocated space (usmblks):  %d\n", mi.usmblks);
		fprintf(file, "Free bytes held in fastbins (fsmblks): %d\n", mi.fsmblks);
		fprintf(file, "Total allocated space (uordblks):      %d\n", mi.uordblks);
		fprintf(file, "Total free space (fordblks):           %d\n", mi.fordblks);
		fprintf(file, "Topmost releasable block (keepcost):   %d\n", mi.keepcost);
		fprintf(file, "------------------- malloc_info -------------------\n");
		malloc_info(0, file);
		fflush(file);
		sleep(10);
	}
}

void
my_init(void)
{
	FILE *traceFile = NULL;
	char *file = getenv("MALLOC_INFO");
	int rc = 0;
	if (NULL != file) {
		traceFile = fopen(file, "w");
	}

	pthread_t *thread = malloc(sizeof(pthread_t));
	rc = pthread_create(thread, NULL, print_malloc_info, traceFile);
	if (-1 == rc) {
		perror("pthread_create");
	}
}
