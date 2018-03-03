/* 2013-08-20: File added by Sony Corporation */
/*
 *@@ TestName - kretp-test-004.c
 *
 *@brief
 * Function Tested      : sys_write using kretprobes feature
 *
 * This testcase will generate the error number 9, the return value
 * returned by the write API when trying to write into the read only file.
 * When the kretprobe is registered for sys_write the return value should
 * be displayed by the kretprobe handler.
 *@pre
 * Test Technique       : Heuristics ( Functional )
 * Compilation          : 1. use Makefile
 *
 * Note : This test program is used after inserting the kretp-test-004.ko
 * kernel module.
 *
 *@criteria
 * The return handler from the kretprobe should return the errno 9 as returned by
 * sys_write.
 *
 *@note
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
  int fd;
  char buf[16];

  fd = open("a.txt", O_CREAT | O_RDONLY);

  if (fd == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  if (write(fd, buf, 15) == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  if ((close(fd)) == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));

  unlink("a.txt");

  return 0;
}
