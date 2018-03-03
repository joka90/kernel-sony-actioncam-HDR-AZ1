/* 2013-08-20: File added by Sony Corporation */
/**
 *
 *@@ TestName - u-008.c
 *
 *@brief
 * Function Tested : A test program for to measure time taken to execute
 *		     gettimeofday().
 *
 * This testcase when executed with the kprobe module k-008, which is
 * having only pre-handler,will give the minimum overhead caused by the
 * kprobes support.
 *
 *@pre
 * Test Technique	: Functional
 * Compilation  	: Use Makefile.
 *
 *@criteria
 *	This test should give the time taken to execute gettimeofday
 *	interms of seconds, nanoseconds and microseconds for each call.
 *
 *
 *@note
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char *argv[])
{
	struct timeval tv, otv;
	int sec = 10;
	unsigned long count;
	struct rusage usage;
	if (argc == 2)
		sec = atoi(argv[1]);
	gettimeofday(&otv, NULL);
	count = 0;
	do {
		gettimeofday(&tv, NULL);
		count++;
	} while ((sec - (tv.tv_sec - otv.tv_sec)) * 1000 >
		 (tv.tv_usec - otv.tv_usec) / 1000);
	getrusage(RUSAGE_SELF, &usage);
	tv = usage.ru_stime;
	tv.tv_sec += usage.ru_utime.tv_sec;
	tv.tv_usec += usage.ru_utime.tv_usec;
	fprintf(stderr,
		"gettimeofday was called %lu times per %d sec: %f nsec per call"
		"\n", count, sec,
		(float)(tv.tv_sec * 1000 * 1000 + tv.tv_usec) / (count / 1000));
	fprintf(stderr,
		"gettimeofday was called %lu times per %d sec: %f usec per call"
		"\n", count, sec,
		(float)(tv.tv_sec * 1000 * 1000 + tv.tv_usec) / (count));
	return 0;
}
