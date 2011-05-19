/* (c) Copyright 2001-2004 Jeff Dike and others
 * Licensed under the GPL, see file COPYING
 *
 * This is uml_console version 2, a tool for querying a User Mode Linux 
 * instance over a local pipe connection. This tool needs to be in sync
 * with the version of the UML kernel.
 *
 * There are a very few local commands that this program knows
 * about, but by default everything gets processed by UML.
 *
 * The uml_mconsole documentation distributed with covers all mconsole
 * commands, so the docs have to be kept in sync with the kernel.
 * In future it should be possible for the docs to come from (or be
 * in common with) something over in the kernel source.
 *
 * If you are looking for the command implementation, go to the
 * files mconsole_kern.c in the Linux kernel source under arch/um.
 * 
 * The program exits with error values of:
 *
 *      0    No error
 *      1    Error     (need better breakdown of error type in future)
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <readline/readline.h>
#include <readline/history.h>

static char uml_name[11];
static struct sockaddr_un sun;

static int do_switch(char *file, char *name)
{
  struct stat buf;

  if(stat(file, &buf) == -1){
    fprintf(stderr, "Warning: couldn't stat file: %s - ", file);
    perror("");
    return(1);
  }
  sun.sun_family = AF_UNIX;
  strncpy(sun.sun_path, file, sizeof(sun.sun_path));
  strncpy(uml_name, name, sizeof(uml_name));
  return(0);
}

static int switch_common(char *name)
{
  char file[MAXPATHLEN + 1], dir[MAXPATHLEN + 1], tmp[MAXPATHLEN + 1], *home;
  int try_file = 1;

  if((home = getenv("HOME")) != NULL){
    snprintf(dir, sizeof(dir), "%s/.uml", home);
    snprintf(file, sizeof(file), "%s/%s/mconsole", dir, name);
    if(strncmp(name, dir, strlen(dir))){
      if(!do_switch(file, name)) return(0);
      try_file = 0;
    }
  }

  snprintf(tmp, sizeof(tmp), "/tmp/uml/%s/mconsole", name);
  if(strncmp(name, "/tmp/uml/", strlen("/tmp/uml/"))){
    if(!do_switch(tmp, name)) return(0);
  }

  if(!do_switch(name, name)) return(0);
  if(!try_file) return(-1);
  return(do_switch(file, name));
}

#define MCONSOLE_MAGIC (0xcafebabe)
#define MCONSOLE_MAX_DATA (512)
#define MCONSOLE_VERSION (2)

struct mconsole_request {
	uint32_t magic;
	uint32_t version;
	uint32_t len;
	char data[MCONSOLE_MAX_DATA];
};

struct mconsole_reply {
	uint32_t err;
	uint32_t more;
	uint32_t len;
	char data[MCONSOLE_MAX_DATA];
};

static char *absolutize(char *to, int size, char *from)
{
  char save_cwd[MAXPATHLEN + 1], *slash;
  int remaining;

  if(getcwd(save_cwd, sizeof(save_cwd)) == NULL) {
    perror("absolutize : unable to get cwd");
    return(NULL);
  }
  slash = strrchr(from, '/');
  if(slash != NULL){
    *slash = '\0';
    if(chdir(from)){
      fprintf(stderr, "absolutize : Can't cd to '%s', ", from);
      *slash = '/';
      perror("");
      return(NULL);
    }
    *slash = '/';
    if(getcwd(to, size) == NULL){
      fprintf(stderr, "absolutize : unable to get cwd of '%s'", from);
      perror("");
      return(NULL);
    }
    remaining = size - strlen(to);
    if(strlen(slash) + 1 > remaining){
      fprintf(stderr, "absolutize : unable to fit '%s' into %d chars\n", from,
	      size);
      return(NULL);
    }
    strcat(to, slash);
  }
  else {
    if(strlen(save_cwd) + 1 + strlen(from) + 1 > size){
      fprintf(stderr, "absolutize : unable to fit '%s' into %d chars\n", from,
	      size);
      return(NULL);
    }
    strcpy(to, save_cwd);
    strcat(to, "/");
    strcat(to, from);
  }
  chdir(save_cwd);
  return(to);
}

static int fix_filenames(char **cmd_ptr)
{
  char *cow_file, *backing_file, *equal, *new, *ptr = *cmd_ptr;
  char full_backing[MAXPATHLEN + 1], full_cow[MAXPATHLEN + 1];
  int len;

  if(strncmp(ptr, "config", strlen("config"))) return(0);
  ptr += strlen("config");

  while(isspace(*ptr) && (*ptr != '\0')) ptr++;
  if(*ptr == '\0') return(0);

  if(strncmp(ptr, "ubd", strlen("ubd"))) return(0);

  while((*ptr != '=') && (*ptr != '\0')) ptr++;
  if(*ptr == '\0') return(0);

  equal = ptr;
  cow_file = ptr + 1;
  while((*ptr != ',') && (*ptr != '\0')) ptr++;
  if(*ptr == '\0'){
    backing_file = cow_file;
    cow_file = NULL;
  }
  else {
    *ptr = '\0';
    backing_file = ptr + 1;
  }

  ptr = absolutize(full_backing, sizeof(full_backing), backing_file);
  backing_file = ptr ? ptr : backing_file;

  if(cow_file != NULL){
    ptr = absolutize(full_cow, sizeof(full_cow), cow_file);
    cow_file = ptr ? ptr : cow_file;
  }

  len = equal - *cmd_ptr;
  len += strlen("=") + strlen(backing_file) + 1;
  if(cow_file != NULL){
    len += strlen(",") + strlen(cow_file);
  }

  new = malloc(len * sizeof(char));
  if(new == NULL) return(0);

  strncpy(new, *cmd_ptr, equal - *cmd_ptr);
  ptr = new + (equal - *cmd_ptr);
  *ptr++ = '=';

  if(cow_file != NULL){
    sprintf(ptr, "%s,", cow_file);
    ptr += strlen(ptr);
  }
  strcpy(ptr, backing_file);

  *cmd_ptr = new;
  return(1);
}

static int default_cmd(int fd, char *command)
{
  struct mconsole_request request;
  struct mconsole_reply reply;
  char name[128];
  int n, free_command, first;

  if((sscanf(command, "%128[^: \f\n\r\t\v]:", name) == 1) &&
     (*(name + 1) == ':')){
    if(switch_common(name)) 
      return(1);
    command = strchr(command, ':');
    *command++ = '\0';
    while(isspace(*command)) command++;
  }

  free_command = fix_filenames(&command);

  request.magic = MCONSOLE_MAGIC;
  request.version = MCONSOLE_VERSION;
  request.len = MIN(strlen(command), sizeof(reply.data) - 1);
  strncpy(request.data, command, request.len);
  request.data[request.len] = '\0';

  if(free_command) free(command);

  if(sendto(fd, &request, sizeof(request), 0, (struct sockaddr *) &sun, 
	    sizeof(sun)) < 0){
    fprintf(stderr, "Sending command to '%s' : ", sun.sun_path);
    perror("");
    return(1);
  }

  first = 1;
  do {
    n = recvfrom(fd, &reply, sizeof(reply), 0, NULL, 0);
    if(n < 0){
      perror("recvmsg");
      return(1);
    }

    if(first){
      if(reply.err) printf("ERR ");
      else printf("OK ");

      first = 0;
    }

    printf("%s", reply.data);
  } while(reply.more);

  printf("\n");
  return(reply.err);
}

char *local_help = 
"Additional local mconsole commands:\n\
    quit - Quit mconsole\n\
    switch <socket-name> - Switch control to the given machine\n\
    log -f <filename> - use contents of <filename> as UML log messages\n\
    mconsole-version - version of this mconsole program\n";

static int help_cmd(int fd, char *command)
{
  default_cmd(fd, command);
  printf("%s", local_help);
  return(0);
}

static int switch_cmd(int fd, char *command)
{
  char *ptr;

  ptr = &command[strlen("switch")];
  while(isspace(*ptr)) ptr++;
  if(switch_common(ptr)) return(1);
  printf("Switched to '%s'\n", ptr);
  return(0);
}

static int log_cmd(int fd, char *command)
{
  int len, max, chunk, input_fd, newline = 0;
  char *ptr, buf[sizeof(((struct mconsole_request *) NULL)->data)];

  ptr = &command[strlen("log")];
  while(isspace(*ptr)) ptr++;

  max = sizeof(((struct mconsole_request *) NULL)->data) - sizeof("log ") - 1;

  if(!strncmp(ptr, "-f", strlen("-f"))){
    ptr = &ptr[strlen("-f")];
    while(isspace(*ptr)) ptr++;
    input_fd = open(ptr, O_RDONLY);
    if(input_fd < 0){
      perror("opening file");
      exit(1);
    }
    strcpy(buf, "log ");
    ptr = buf + strlen(buf);
    while((len = read(input_fd, ptr, max)) > 0){
      ptr[len] = '\0';
      default_cmd(fd, buf);
      newline = (ptr[len - 1] == '\n');
    }
    if(len < 0){
      perror("reading file");
      exit(1);
    }
  }
  else {
    len = strlen(ptr);
    while(len > 0){
      chunk = MIN(len, max);
      sprintf(buf, "log %.*s", chunk, ptr);
      default_cmd(fd, buf);
      newline = (ptr[chunk - 1] == '\n');

      len -= chunk;
      ptr += chunk;
    }
  }
  if(!newline){
    sprintf(buf, "log \n");
    default_cmd(fd, buf);
  }
  return(0);
}

static int quit_cmd(int fd, char *command)
{
  exit(0);
}

static int mversion_cmd(int fd, char *command)
{
  printf("uml_mconsole client version %d\n", MCONSOLE_VERSION);
  return(0);
}

struct cmd {
  char *command;
  int (*proc)(int, char *);
};

static struct cmd cmds[] = {
  { "quit", quit_cmd },
  { "help", help_cmd },
  { "switch", switch_cmd },
  { "log", log_cmd },
  { "mconsole-version", mversion_cmd },
  { NULL, default_cmd }
  /* default_cmd means "send it to the UML" */
};

/* sends a command */
int issue_command(int fd, char *command)
{
  char *ptr;
  int i = 0;

  /* Trim trailing spaces left by readline's filename completion */
  ptr = &command[strlen(command) - 1];
  while(isspace(*ptr)) *ptr-- = '\0';
    
  for(i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++){
    if((cmds[i].command == NULL) || 
       !strncmp(cmds[i].command, command, strlen(cmds[i].command))){
      return((*cmds[i].proc)(fd, command));
    }
  }

  /* Should never get here, considering the NULL test above will match the last
   * entry of cmds */
  return(0);
}

/* sends a command in argv style array */
int issue_commandv(int fd, char **argv)
{
  char *command;
  int len = -1, i = 0, status;

  len = 1;  /* space for trailing null */
  for(i = 0; argv[i] != NULL; i++)
    len += strlen(argv[i]) + 1;  /* space for space */

  command = malloc(len);
  if(command == NULL){
    perror("issue_command");
    return(1);
  }
  command[0] = '\0';

  for(i = 0; argv[i] != NULL; i++) {
    strcat(command, argv[i]);
    if(argv[i+1] != NULL) strcat(command, " ");
  }

  status = issue_command(fd, command);

  free(command);

  return(status);
}

static void Usage(void)
{
  fprintf(stderr, "Usage : uml_mconsole socket-name [command]\n");
  exit(1);
}

int main(int argc, char **argv)
{
  struct sockaddr_un here;
  char *sock;
  int fd;

  if(argc < 2) Usage();
  strcpy(uml_name, "[None]");
  sock = argv[1];
  switch_common(sock);

  if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }
  here.sun_family = AF_UNIX;
  memset(here.sun_path, 0, sizeof(here.sun_path));

  sprintf(&here.sun_path[1], "%5d", getpid());
  if(bind(fd, (struct sockaddr *) &here, sizeof(here)) < 0){
    perror("bind");
    exit(1);
  }

  if(argc>2)
    exit(issue_commandv(fd, argv+2));

  while(1){
    char *command, prompt[1 + sizeof(uml_name) + 2 + 1];

    sprintf(prompt, "(%s) ", uml_name);
    command = readline(prompt);
    if(command == NULL) break;

    if(*command) add_history(command);

    issue_command(fd, command);
    free(command);
  }
  printf("\n");
  exit(0);
}
