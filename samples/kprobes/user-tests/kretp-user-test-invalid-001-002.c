/* 2013-08-20: File added by Sony Corporation */
/*
 *@@ TestName - kretp-user-test-invalid-001-002.c
 *
 *@brief
 * Function Tested      : sys_open and sys_close using kretprobes feature
 *
 * This testcase will generate the error numbers 2, 9, 20 the return values
 * returned by the open, close, opendir API's respectively. When the kretprobe
 * is registered for the sys_open and sys_close symbols the return values should
 * be displayed by the kretprobe handler.
 *
 *@pre
 * Test Technique       : Heuristics ( Functional )
 * Compilation          : 1. use Makefile
 *
 * Note : This test program is used after inserting the kretp-test-001.ko and
 * kretp-test-002.ko kernel modules.
 *
 *@criteria
 * When the kretprobe is registered for the sys_open and sys_close symbols the
 * return values should be displayed by the kretprobe handler.
 *
 *@note
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
  int fd;

  fd = open("a.txt", O_RDONLY);

  if (fd == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  if ((close(fd)) == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  fd = open("a.txt", O_CREAT);

  if (fd == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  if (opendir("a.txt") == 0)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  unlink("a.txt");

  return 0;
}
