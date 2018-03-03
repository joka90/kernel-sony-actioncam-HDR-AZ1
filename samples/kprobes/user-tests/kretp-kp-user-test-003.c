/* 2013-08-20: File added by Sony Corporation */
/**
 *
 *@@ TestName - kretp-kp-user-test-003.c
 *
 *@brief
 * Function Tested	: read()
 * 1. Read a fresh 1KB to the buffer from the file every time.
 * 2. Increment the count for each read
 *
 * Note : This testcase will insert the kretp-kp-test-003.ko which
 * has a kprobe and a kretprobe registered for sys_read function.
 *
 *@pre
 * Test Technique	: Heuristics ( Functional )
 * Compilation  	: 1. use Makefile
 *
 *@criteria
 * This test gives user read count
 *
 *@note
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#define BUF_SIZE  (1 * 1024 *  1024)
#define UPDATE_MAX 100
#define SIZE   (1 * 1024)

static int user_read_count = 0;

int main()
{
	char file_buf[SIZE];
	int i = 1, fd, j;

	printf("%s %d read api test\n", __FILE__, __LINE__);

	fd = open("/tmp/stat-read-doc.txt", O_CREAT | O_RDWR, 0777);

	for (j = 0; j < SIZE; j++) {
		file_buf[j] = 'a';
	}

	if ((lseek(fd, (UPDATE_MAX * BUF_SIZE), SEEK_SET)) == -1) {
		perror("Could not read file offset\n");
		printf("%s %d lseek() TEST FAIL\n", __FILE__, __LINE__);
		return 0;
	}

	if ((write(fd, file_buf, sizeof(file_buf))) == -1) {
		printf("%s %d Error in write \n", __FILE__, __LINE__);
	}

	if ((lseek(fd, 0, SEEK_SET)) == -1) {
		perror("Could not read file offset\n");
		printf("%s %d lseek() TEST FAIL\n", __FILE__, __LINE__);
		return 0;
	}

	system("modprobe kretp-kp-test-003");

	while (1) {

		if (read(fd, (void *)file_buf, (SIZE)) == -1) {
			printf("%s %d, Read failed\n", __FILE__, __LINE__);
			break;
		}

		user_read_count++;

		if (user_read_count == UPDATE_MAX) {
			break;
		}

		i++;
	}

	system("rmmod kretp-kp-test-003");

	printf("%s %d, user_read_count %d \n", __FILE__, __LINE__, user_read_count);

	close(fd);
	remove("/tmp/stat-read-doc.txt");
	return 0;
}
