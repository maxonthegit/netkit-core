#include <stddef.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

void fail(int fd)
{
  char c = 0;

  if(write(fd, &c, sizeof(c)) != sizeof(c))
    perror("Writing failure byte");
  exit(1);
}

int do_exec(char **args, int need_zero)
{
  int pid, status;

  if((pid = fork()) == 0){
    execvp(args[0], args);
    exit(1);
  }
  else if(pid < 0){
    perror("fork failed");
    return(-1);
  }
  if(waitpid(pid, &status, 0) < 0){
    perror("execvp");
    return(-1);
  }
  if(need_zero && (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))){
    printf("'%s' didn't exit with status 0\n", args[0]);
    return(-1);
  }
  return(0);
}

#define BUF_SIZE 1500

main(int argc, char **argv)
{
  char *dev = argv[1];
  int fd = atoi(argv[2]);
  char *gate_addr = argv[3];
  char *remote_addr = argv[4];
  int pid, status;
  char *ifconfig_argv[] = { "ifconfig", dev, "arp", gate_addr, "up",
			    "mtu", "1484", NULL };
  char *route_argv[] = { "route", "add", "-host", remote_addr, "gw", 
			 gate_addr, NULL };
  char *arp_argv[] = { "arp", "-Ds", remote_addr, "eth0", "pub", NULL };
  char *forw_argv[] = { "bash",  "-c", 
			"echo 1 > /proc/sys/net/ipv4/ip_forward", NULL };
  char dev_file[sizeof("/dev/tapxxxx\0")], c;
  int tap, *fdptr;

  setreuid(0, 0);

  sprintf(dev_file, "/dev/%s", dev);
  if((tap = open(dev_file, O_RDWR | O_NONBLOCK)) < 0){
    perror("open");
    fail(fd);
  }

  if(do_exec(ifconfig_argv, 1)) fail(fd);
  if(do_exec(route_argv, 0)) fail(fd);
  do_exec(arp_argv, 0);
  do_exec(forw_argv, 0);

  /* On UML : route add -net 192.168.0.0 gw 192.168.0.4 netmask 255.255.255.0
   */

  c = 1;
  if(write(fd, &c, sizeof(c)) != sizeof(c)){
    perror("write");
    fail(fd);
  }

  while(1){
    fd_set fds, except;
    char buf[BUF_SIZE];
    int from, to, n, max;

    FD_ZERO(&fds);
    FD_SET(tap, &fds);
    FD_SET(fd, &fds);
    except = fds;
    max = ((tap > fd) ? tap : fd) + 1;
    if(select(max, &fds, NULL, &except, NULL) < 0){
      perror("select");
      continue;
    }
    if(FD_ISSET(tap, &fds)){
      from = tap;
      to = fd;
    }
    else if(FD_ISSET(fd, &fds)){
      from = fd;
      to = tap;
    }
    else continue;
    n = read(from, buf, sizeof(buf));
    if(n == 0){
      exit(0);
    }
    else if(n < 0) perror("read");
    n = write(to, buf, n);
    if(n < 0) perror("write");
  }
  return(0);
}
