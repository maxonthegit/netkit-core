#include <stdio.h>
#include <unistd.h>
#include <errno.h>

static void Usage(void)
{
  fprintf(stderr, "Usage : jail_uml jail-directory uid "
	  "uml-command-line ...\n");
  exit(1);
}

int main(int argc, char **argv)
{
  char *dir, *end;
  int uid;

  if(geteuid() != 0){
    fprintf(stderr, "jail_uml must be run as root\n");
    exit(1);
  }

  if(argc < 3) Usage();
  dir = argv[1];
  uid = strtoul(argv[2], &end, 0);
  if(*end != '\0') Usage();
  argc -= 3;
  argv += 3;

  if(chdir(dir)){
    perror("chdir");
    exit(1);
  }

  if(chroot(".")){
    perror("chroot");
    exit(1);
  }

  if(setuid(uid)){
    perror("setuid");
    exit(1);
  }

  execv(argv[0], argv);
  fprintf(stderr, "execve of %s failed : ", argv[0]);
  perror("");
  exit(1);
}
