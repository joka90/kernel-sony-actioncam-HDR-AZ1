/* 2013-08-20: File added by Sony Corporation */
/**
 *
 *@@ TestName - u-006.c
 *
 *@brief
 * Function Tested	: write()
 * 1. Write 1KB to file from the buffer every time.
 * 2. Increment the count for each write
 *
 *@pre
 * Test Technique	: Heuristics ( Functional )
 * Compilation  	: 1. Use Makefile
 *
 *@criteria
 * This test gives user write count
 *
 *@note
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#define BUF_SIZE  (10 * 1024)
#define SIZE   1024
#define UPDATE_MAX 100

static int user_write_count = 0;

int main()
{
	char file_buf[BUF_SIZE];
	int i = 1, fd;

	printf("%s %d, write api test\n",  __FILE__, __LINE__);

	fd = open("/tmp/stat-write-doc.txt", O_CREAT | O_RDWR, 0777);
	system("modprobe k-006");

	while (1) {

		if (write(fd, &file_buf, (SIZE)) == -1) {
			printf("%s %d, write failed\n", __FILE__, __LINE__);
			return 0;
		}
		user_write_count++;

		if (user_write_count == UPDATE_MAX) {
			break;
		}
		i++;
	}

	system("rmmod k-006");
	printf("%s %d, user_write_count %d \n", __FILE__, __LINE__, user_write_count);

	close(fd);
	remove("/tmp/stat-write-doc.txt");
	return 0;
}
