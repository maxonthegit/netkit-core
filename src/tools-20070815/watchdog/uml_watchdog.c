#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MCONSOLE_MAGIC (0xcafebabe)
#define MCONSOLE_MAX_DATA (512)
#define MCONSOLE_VERSION 2

struct mconsole_notify {
	unsigned long magic;
	int version;	
	enum { MCONSOLE_SOCKET, MCONSOLE_PANIC, MCONSOLE_HANG } type;
	int len;
	char data[MCONSOLE_MAX_DATA];
};

static char *sock_name = NULL;

static void notify(int sig)
{
  struct sockaddr_un target;
  struct mconsole_notify packet;
  int sock, len, n;

  sock = socket(PF_UNIX, SOCK_DGRAM, 0);
  if(sock < 0){
    perror("socket");
    exit(1);
  }

  target.sun_family = AF_UNIX;
  strcpy(target.sun_path, sock_name);

  packet.magic = MCONSOLE_MAGIC;
  packet.version = MCONSOLE_VERSION;
  packet.type = MCONSOLE_HANG;
  packet.len = 0;

  len = sizeof(packet) + packet.len - sizeof(packet.data);
  n = sendto(sock, &packet, len, 0, (struct sockaddr *) &target, 
	     sizeof(target));
  if(n < 0)
    perror("sendto");
  exit(1);
}

static int tracing_pid = -1;

static void kill_tracer(int sig)
{
  kill(tracing_pid, SIGINT);
  sleep(1);
  kill(tracing_pid, SIGINT);
  sleep(1);
  kill(tracing_pid, SIGKILL);
  exit(1);
}

int main(int argc, char **argv)
{
  void (*handler)(int);
  int n;
  char c, *end;

  if(argc < 3) exit(1);

  if(!strcmp(argv[1], "-pid")){
    tracing_pid = strtol(argv[2], &end, 0);
    if(*end != '\0') exit(1);
    if(kill(tracing_pid, 0)) exit(1);
    handler = kill_tracer;
  }
  else if(!strcmp(argv[1], "-mconsole")){
    sock_name = argv[2];
    handler = notify;
  }
  else exit(1);

  if(signal(SIGALRM, handler)) exit(1);

  if(write(1, &c, sizeof(c)) != sizeof(c)) exit(1);

  while(1){
    alarm(60);
    n = read(0, &c, sizeof(c));
    if(n == 0) exit(0);
    else if(n < 0) exit(1);
  }
}
