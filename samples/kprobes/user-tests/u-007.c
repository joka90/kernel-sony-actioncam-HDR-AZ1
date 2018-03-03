/* 2013-08-20: File added by Sony Corporation */
/**
 *
 *@@ TestName - u-007.c
 *
 *@brief
 * Function Tested	: u-007
 *
 *  Check the reentrancy of the mem related functions memset,memchr,memcpy,
 *  memcmp functions.
 *
 *
 *@pre
 * Test Technique	: Heuristics ( Functional )
 * Compilation  	: 1. Use Maekfile
 *
 *@criteria
 * The mem related functions are reentrant in nature, and hence it should be
 * proved without using any locks
 *
 *
 *@note
 *
 *
 */

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define SIZE (1024/4 )

#define MAX_THREAD  15
#define LOOP 10

struct thr_parms_u_007 {
	struct thr_parms_u_007 *next;
	pthread_t thread;
	int id;
	char *buff;
};

struct thr_parms_u_007 *u_007_thr_list = NULL;
int u_007_result = 0;
char *u_007_map;
int kprobe_malloc_count = 0;
int kprobe_free_count = 0;

void print_u_007_result(void)
{
	printf("################################\n");
	printf("\n\n\n\n");
	printf("%s %d, kprobe_malloc_count:%d kprobe_free_count=%d \n", __FILE__, __LINE__,
	       kprobe_malloc_count, kprobe_free_count);
	printf("\n\n\n\n");
	printf("################################\n");

	printf(__FILE__ " TEST %s(%d)\n",
	       (u_007_result == 0) ? "PASS" : "FAIL", u_007_result);
}

void *u_007_mem_loop_handler(void *arg)
{
	struct thr_parms_u_007 *p = (struct thr_parms_u_007 *)arg;
	int ret, i, j;
	int count1 = 0;
	char *new_map;
	char *ret_str1;
	char *ret_str2;

	new_map = malloc(BUF_SIZE * sizeof(char));
	kprobe_malloc_count++;
	for (j = 0; j < LOOP; j++) {
		memcpy(p->buff, u_007_map, BUF_SIZE);

		if (strlen(p->buff) != strlen(u_007_map)) {
			printf("\n%s %d Error strlen does not match for Thread "
			       "Id = %d \n", __FILE__, __LINE__, p->id);
			u_007_result = 4;
			break;
		}

		ret = memcmp(u_007_map, p->buff, BUF_SIZE);
		if (ret != 0) {
			printf("\n%s %d memcmp:Error in copying for Thread "
			       "Id = %d \n", __FILE__, __LINE__, p->id);
			u_007_result = 5;
			break;
		} else {
			printf("\n%s %d Successful comparison for Thread "
			       "Id = %d \n", __FILE__, __LINE__, p->id);
		}

		new_map = memmove(new_map, u_007_map, BUF_SIZE);

		if (strlen(p->buff) != strlen(new_map)) {
			printf("\n%s %d Error strlen does not match for Thread "
			       "Id = %d \n", __FILE__, __LINE__, p->id);
			u_007_result = 6;
			break;
		}

		ret = memcmp(new_map, u_007_map, BUF_SIZE);
		if (ret != 0) {
			printf("\n%s %d memcmp:Error in memmove for Thread "
			       "Id = %d \n", __FILE__, __LINE__, p->id);
			u_007_result = 7;
			break;
		} else {
			printf("\n%s %d Successful comparison for Thread "
			       "Id = %d \n", __FILE__, __LINE__, p->id);
		}
	}
	memset(new_map, '1', 1024);
	ret_str2 = new_map;
	for (i = 0; i < BUF_SIZE; i++) {
		ret_str1 = memchr(ret_str2, '1', 1024);
		if (ret_str1 == NULL) {
			printf("\n%s %d memchr:fail  for Thread Id = %d \n",
			       __FILE__, __LINE__, p->id);
			u_007_result = 8;
			break;
		} else {
			count1++;
			ret_str2 = ret_str1;
		}
	}
	if (count1 == BUF_SIZE) ;
	else {
		printf("\n%s %d memchr:fails  for Thread Id = %d  count %d\n",
		       __FILE__, __LINE__, p->id, count1);
		u_007_result = 9;
		return NULL;
	}

	free(new_map);
	kprobe_free_count++;
	return NULL;
}

int main()
{
	int i, j, ret;
	struct thr_parms_u_007 *m, *p;
	char s[32] = "1234567891011121314151617181920";
	char *string, *buf, *buf1;
	int fd;
	string = s;

	system("modprobe k-007");

	printf("%s %d, SCEN_U_007: \n", __FILE__, __LINE__);

	if ((fd =
	     open("/tmp/msynctest", O_RDWR | O_CREAT | O_TRUNC, 0600)) == -1) {
		printf("\t%s %d: open failed ,TEST FAIL\n", __FILE__, __LINE__);
		return 0;
	}

	u_007_map =
	    mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE,
		 fd, 0);

	if (u_007_map == MAP_FAILED) {
		printf("%s %d Test Fail:  Error at mmap: %s\n", __FILE__,
		       __LINE__, strerror(errno));
	}

	buf = malloc(BUF_SIZE * sizeof(char));
	kprobe_malloc_count++;

	for (j = 0; j < BUF_SIZE; j++) {
		buf[j] = string[0];
		string++;
		if ((j % 24) == 0) {
			string = s;
		}
	}
	buf[BUF_SIZE - 1] = '\0';

	u_007_map = buf;

	for (i = 0; i < MAX_THREAD; i++) {
		struct thr_parms_u_007 *p =
		    malloc(sizeof(struct thr_parms_u_007));
		kprobe_malloc_count++;

		if (!p) {
			u_007_result = 1;
			break;
		}
		p->id = i + 1;
		p->next = u_007_thr_list;
		u_007_thr_list = p;

		p->buff = (char *)malloc(BUF_SIZE * sizeof(char));

		kprobe_malloc_count++;
		ret =
		    pthread_create(&p->thread, NULL, u_007_mem_loop_handler, p);
		if (ret) {
			u_007_result = 3;
			break;
		}
		usleep(1);
	}

	for (m = u_007_thr_list; m != NULL;) {
		ret = pthread_join(m->thread, NULL);
		if (ret) {
			printf("\n%s %d Thread  %d is not joined..\n", __FILE__,
			       __LINE__, m->id);
		}
		p = m;
		m = m->next;
		free(p->buff);
		kprobe_free_count++;
		free(p);
		kprobe_free_count++;
	}

	free(buf);
	kprobe_free_count++;
	munmap(u_007_map, BUF_SIZE);
	close(fd);

	for (j = 0; j < 4; j++) {
		buf1 = malloc(SIZE * sizeof(char));
		kprobe_malloc_count++;
		free(buf1);
		kprobe_free_count++;
	}
	print_u_007_result();
	system("rmmod k-007");
	return 0;
}
