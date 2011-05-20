/* port-helper

Used by the port and xterm console channels for User Mode Linux.

Tells UML "here is a file descriptor for my stdin as given to me by 
xterm or telnetd". Once UML has that (with os_rcv_fd()) UML opens it
for read and write, and the console is functional.

(c) Jeff Dike 2002-2004

*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

extern void send_fd(int fd, int target, struct sockaddr *to, int to_len, 
		    void *msg, int msg_len);

/* for xterm we don't have an open socket, we only have the name
 of a file used as a Unix socket by UML. So we need to open it. */
static int open_socket(char *name)
{
  struct sockaddr_un sock;
  int fd;

  if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }

  sock.sun_family = AF_UNIX;
  memset(sock.sun_path, 0, sizeof(sock.sun_path));
  sprintf(&sock.sun_path[1], "%5d", getpid());

  if(bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0){
    perror("bind");
    exit(1);
  }

  snprintf(sock.sun_path, sizeof(sock.sun_path), "%s", name);
  if(connect(fd, (struct sockaddr *) &sock, sizeof(sock))){
    perror("connect");
    exit(1);
  }

  return(fd);
}

int main(int argc, char **argv)
{
  int fd, pid;

  if((argc > 1) && !strcmp(argv[1], "-uml-socket")) {
	/* inherited a filename not an open fd */
	fd = open_socket(argv[2]);
  } else {
	/* inherited fd of the listening TCP socket */
	fd = 3;
  }

  signal(SIGHUP, SIG_IGN);
  if(ioctl(0, TIOCNOTTY, 0) < 0)
    perror("TIOCNOTTY failed in port-helper, check UML's exec call for xterm or telnetd");
  pid = getpid();
  send_fd(0, fd, NULL, 0, &pid, sizeof(pid));
  pause();
  return(0);
}
