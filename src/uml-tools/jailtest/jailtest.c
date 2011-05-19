#include <stdio.h>
#include <signal.h>
#include <wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define KERNEL_ADDRESS ((unsigned long *) (0xa0800000))

int main(int argc, char **argv)
{
  int pid, status, fd, n;

  printf("Trying to directly write %p...", KERNEL_ADDRESS);
  fflush(stdout);
  if((pid = fork()) != 0){
    if(waitpid(pid, &status, 0) < 0){
      perror("waitpid");
      exit(1);
    }
    if(!WIFSIGNALED(status) || (WTERMSIG(status) != SIGSEGV))
      printf("No segv (status = 0x%x) - BAD!\n", status);
    else printf("Segv - GOOD\n");
  }
  else {
    *KERNEL_ADDRESS = 0xdeadbeef;
    exit(1);
  }

  printf("Trying to write %p via read...", KERNEL_ADDRESS);
  if((fd = open("/etc/passwd", O_RDONLY)) < 0){
    perror("open");
    exit(1);
  }
  n = read(fd, (char *) KERNEL_ADDRESS, sizeof(*KERNEL_ADDRESS));
  if((n > 0) || (errno != EFAULT))
    printf("Didn't return EFAULT - BAD!\n");
  else printf("Returned EFAULT - GOOD\n");

  printf("Trying to open /dev/mem...");
  fd = open("/dev/mem", O_WRONLY);
  if((fd >= 0) || (errno != EPERM))
    printf("Didn't fail with EPERM (fd = %d, errno = %d) - BAD!\n", fd, 
	   errno);
  else printf("Failed with EPERM - GOOD\n");

  printf("Trying to open /dev/kmem...");
  fd = open("/dev/kmem", O_WRONLY);
  if((fd >= 0) || (errno != EPERM))
    printf("Didn't fail with EPERM (fd = %d, errno = %d) - BAD!\n", fd, 
	   errno);
  else printf("Failed with EPERM - GOOD\n");

  return(0);
}
