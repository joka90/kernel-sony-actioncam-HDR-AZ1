/* 2013-08-20: File added by Sony Corporation */
/*
 *@@ TestName - kretp-user-test-valid-001-002.c
 *
 *@brief
 * Function Tested      : sys_open and sys_close using kretprobes feature
 *
 * This testcase will generate the valid return values as returned by the open,
 * close, mkdir, opendir, closedir, rmdir and unlink API's respectively. When the
 * kretprobe is registered for the sys_open and sys_close symbols the return values
 * should be displayed by the kretprobe handler.
 *
 * Note : This test program is used after inserting the kretp-test-001.ko and
 * kretp-test-002.ko kernel modules.
 *
 *@pre
 * Test Technique       : Heuristics ( Functional )
 * Compilation          : 1. use Makefile
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
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
  int fd;
  DIR *p;

  fd = open("a.txt", O_CREAT | O_RDONLY);

  if (fd == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d open is successful\n", __FILE__, __LINE__);

  if ((close(fd)) == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d close is successful\n", __FILE__, __LINE__);


  if (mkdir("a", 0777) == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d mkdir is successful\n", __FILE__, __LINE__);

  p = opendir("a");

  if (p == 0)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d opendir is successful\n", __FILE__, __LINE__);

  if (closedir(p) == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d closedir is successful\n", __FILE__, __LINE__);

  if (rmdir("a") == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d rmdir is successful\n", __FILE__, __LINE__);

  if (unlink("a.txt") == -1)
    printf("FILE:%s LINE:%d ERROR:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
  else
    printf("FILE:%s LINE:%d unlink is successful\n", __FILE__, __LINE__);

  return 0;
}
