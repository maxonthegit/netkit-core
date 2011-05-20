/* Copyright 2001 Jeff Dike and others
 * Licensed under the GPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define CURRENT_VERSION (4)

extern void ethertap_v0(int argc, char **argv);
extern void ethertap_v1_v2(int argc, char **argv);
extern void ethertap_v1_v2(int argc, char **argv);
extern void ethertap_v3(int argc, char **argv);
extern void ethertap_v4(int argc, char **argv);

void (*ethertap_handlers[])(int argc, char **argv) = {
  ethertap_v0, 
  ethertap_v1_v2,
  ethertap_v1_v2,
  ethertap_v3,
  ethertap_v4,
};

extern void slip_v0_v2(int argc, char **argv);
extern void slip_v3(int argc, char **argv);
extern void slip_v4(int argc, char **argv);

void (*slip_handlers[])(int argc, char **argv) = { 
  slip_v0_v2, 
  slip_v0_v2,
  slip_v0_v2, 
  slip_v3, 
  slip_v4, 
};

#ifdef TUNTAP

extern void tuntap_v2(int argc, char **argv);
extern void tuntap_v3(int argc, char **argv);
extern void tuntap_v4(int argc, char **argv);

void (*tuntap_handlers[])(int argc, char **argv) = {
  NULL,
  NULL,
  tuntap_v2,
  tuntap_v3,
  tuntap_v4,
};

#endif

extern void add_address_v4(int argc, char **argv);

void (*add_handlers[])(int argc, char **argv) = {
  NULL,
  NULL,
  NULL,
  NULL,
  add_address_v4,
};

extern void del_address_v4(int argc, char **argv);

void (*del_handlers[])(int argc, char **argv) = {
  NULL,
  NULL,
  NULL,
  NULL,
  del_address_v4,
};

static void handler(int sig)
{
}

int main(int argc, char **argv)
{
  char *version = argv[1];
  char *transport = argv[2];
  void (**handlers)(int, char **);
  struct sigaction sa;
  char *out;
  int n = 3;
  unsigned int v;

  if(sigaction(SIGCHLD, NULL, &sa) < 0){
    perror("sigaction");
    exit(1);
  }
  sa.sa_handler = handler;
  sa.sa_flags &= ~SA_NOCLDWAIT;
  sa.sa_flags |= SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) < 0){
    perror("sigaction");
    exit(1);
  }

  if(argc < 3){
    fprintf(stderr, 
	    "uml_net : bad argument list - if you're running uml_net by\n"
	    "hand, don't bother.  uml_net is run automatically by UML when\n"
	    "necessary.\n");
    exit(1);
  }
  setenv("PATH", "/bin:/usr/bin:/sbin:/usr/sbin", 1);
  v = strtoul(version, &out, 0);
  if(out != version){
    if(v > CURRENT_VERSION){
      fprintf(stderr, "Version mismatch - requested version %d, uml_net "
	      "supports up to version %d\n", v, CURRENT_VERSION);
      exit(1);
    }
  }
  else {
    v = 0;
    transport = version;
    n = 2;
  }
  if(!strcmp(transport, "ethertap")) handlers = ethertap_handlers;
  else if(!strcmp(transport, "slip")) handlers = slip_handlers;
#ifdef TUNTAP
  else if(!strcmp(transport, "tuntap")) handlers = tuntap_handlers;
#endif
  else if(!strcmp(transport, "add")) handlers = add_handlers;
  else if(!strcmp(transport, "del")) handlers = del_handlers;
  else {
    printf("Unknown transport : '%s'\n", transport);
    exit(1);
  }
  if(handlers[v] != NULL) (*handlers[v])(argc - n, &argv[n]);
  else {
    printf("No version #%d handler for '%s'\n", v, transport);
    exit(1);
  }
  return(0);
}
