/* jail a uml into a directory.
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

static void Usage(void)
{
  fprintf(stderr, "Usage : jail_uml jail-directory user "
	  "uml-command-line ...\n");
  fprintf(stderr, "    or: jail_uml jail-directory uid "
	  "uml-command-line ...\n\n");
  fprintf(stderr, "If the user is not found, it's assumed to be a uid.\n");
  exit(1);
}

int main(int argc, char **argv)
{
  char *dir, *end;
  char *user;
  struct passwd *pw;
  int uid, gid=99;
  gid_t gidset[1];
  gidset[0]=gid;

  if(geteuid() != 0){
    fprintf(stderr, "jail_uml must be run as root\n");
    exit(1);
  }

  if(argc < 3) Usage();
  dir = argv[1];
  user = argv[2];
  
  // get users password information
  pw = getpwnam (user);
  if (pw == NULL){
    uid = strtoul(argv[2], &end, 0);
    if(*end != '\0') Usage();
    setgroups(1, gidset);
  } else {
    // try to init groups
    initgroups (pw->pw_name, pw->pw_gid); 
    uid = pw->pw_uid;
    gid = pw->pw_gid;
  }

  // if(*end != '\0') Usage();
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

  if(setgid(gid)){
    perror("setgid");
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
