/* Copyright 2001 Jeff Dike and others
 * Licensed under the GPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "host.h"
#include "output.h"

static void fail(int fd)
{
  char c = 0;

  if(write(fd, &c, sizeof(c)) != sizeof(c))
    perror("Writing failure byte");
}

void output_fail(struct output *output, int fd)
{
  fail(fd);
  write_output(fd, output);
  exit(1);
}

/* Shouldn't there be some other way to figure out the major and minor
 * number of the tap device other than hard-wiring it here? Does FreeBSD
 * have a 'tap' device? Should we have some kind of #ifdef here?
 */
#define TAP_MAJOR 36
#define TAP_MINOR 16  /* plus whatever tap device it was. */

static int maybe_insmod(char *dev, struct output *output)
{
  struct ifreq ifr;
  int fd, unit;
  char unit_buf[sizeof("unit=nnn\0")];
  char ethertap_buf[sizeof("ethertapnnn\0")];
  char *ethertap_argv[] = { "modprobe", "ethertap", unit_buf, "-o", 
			    ethertap_buf, NULL };
  char *netlink_argv[] = { "modprobe", "netlink_dev", NULL };
  char buf[256];
  
  if((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
    output_errno(output, "socket");
    return(-1);
  }
  strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
  ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
  if(ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) return(0);
  if(errno != ENODEV){
    output_errno(output, "SIOCGIFFLAGS on tap device");
    return(-1);
  }
  if(sscanf(dev, "tap%d", &unit) != 1){
    snprintf(buf, sizeof(buf), "failed to get unit number from '%s'\n", dev);
    add_output(output, buf, -1);
    return(-1);
  }
  snprintf(unit_buf, sizeof(unit_buf) - 1, "unit=%d", unit);
  snprintf(ethertap_buf, sizeof(ethertap_buf) - 1, "ethertap%d", unit);
  do_exec(netlink_argv, 0, output);
  return(do_exec(ethertap_argv, 0, output));
}

#define BUF_SIZE 1500

#define max(i, j) (((i) > (j)) ? (i) : (j))

static void ethertap(char *dev, int data_fd, int control_fd, char *gate, 
		     char *remote, int collect_output, 
		     int (*control_cb)(int, char *, struct output *))
{
  struct output output = INIT_OUTPUT, *o = NULL;
  int ip[4];
  char *ifconfig_argv[] = { "ifconfig", dev, "arp", "mtu", "1500", gate,
			    "netmask", "255.255.255.255", "up", NULL };
  char *down_argv[] = { "ifconfig", dev, "0.0.0.0", "down", NULL };
  char dev_file[sizeof("/dev/tapxxxx\0")], buf[256], c;
  int tap, minor;

  if(collect_output) o = &output;
  if(setreuid(0, 0) < 0){
    output_errno(o, "setreuid");
    output_fail(o, control_fd);
  }
  if(gate != NULL){
    sscanf(gate, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
    if(maybe_insmod(dev, o)) output_fail(o, control_fd);

    if(do_exec(ifconfig_argv, 1, o)) output_fail(o, control_fd);

    if((remote != NULL) && route_and_arp(dev, remote, NULL, 1, o)) 
      output_fail(o, control_fd);

    forward_ip(o);
  }

  if(!is_a_device(dev)){
    add_output(o, "Device doesn't contain only alphanumeric characters\n", -1);
    output_fail(o, control_fd);
  }
  snprintf(dev_file, sizeof(dev_file) - 1, "/dev/%.7s", dev);

  /* do a mknod on it in case it doesn't exist. */

  if(sscanf(dev_file, "/dev/tap%d", &minor) != 1){
    snprintf(buf, sizeof(buf), "failed to get unit number from '%s'\n", 
	     dev_file);
    add_output(o, buf, -1);
    output_fail(o, control_fd);
  }
  minor += TAP_MINOR;
  mk_node(dev_file, TAP_MAJOR, minor); 

  if((tap = open(dev_file, O_RDWR | O_NONBLOCK)) < 0){
    output_errno(o, "open");
    output_fail(o, control_fd);
  }

  c = 1;
  if(write(control_fd, &c, sizeof(c)) != sizeof(c)){
    output_errno(o, "write");
    output_fail(o, control_fd);
  }
  write_output(control_fd, o);

  while(1){
    fd_set fds, except;
    char buf[BUF_SIZE];
    int n, max_fd;

    FD_ZERO(&fds);
    FD_SET(tap, &fds);
    FD_SET(data_fd, &fds);
    FD_SET(control_fd, &fds);
    except = fds;
    max_fd = max(max(tap, data_fd), control_fd) + 1;
    if(select(max_fd, &fds, NULL, &except, NULL) < 0){
      perror("select");
      continue;
    }
    if(FD_ISSET(tap, &fds)){
      n = read(tap, buf, sizeof(buf));
      if(n == 0) break;
      else if(n < 0){
	perror("read");
	continue;
      }
      n = send(data_fd, buf, n, 0);
      if((n < 0) && (errno != EAGAIN)){
	perror("send");
	break;
      }
    }
    else if(FD_ISSET(data_fd, &fds)){
      n = recvfrom(data_fd, buf, sizeof(buf), 0, NULL, NULL);
      if(n == 0) break;
      else if(n < 0) perror("recvfrom");
      n = write(tap, buf, n);
      if(n < 0) perror("write");      
    }
    else if(FD_ISSET(control_fd, &fds)){
      if((*control_cb)(control_fd, dev, o)) break;
    }
  }
  if(gate != NULL) do_exec(down_argv, 0, NULL);
  if(remote != NULL) no_route_and_arp(dev, remote, NULL, NULL);
}

struct addr_change_v1_v3 {
  enum change_type what;
  unsigned char addr[4];
};

static int control_v1_v3(int fd, char *dev, struct output *output)
{
  struct addr_change_v1_v3 change;
  int n;

  n = read(fd, &change, sizeof(change));
  if(n == sizeof(change)) 
    address_change(change.what, change.addr, dev, NULL, output);
  else if(n == 0) return(1);
  else {
    fprintf(stderr, "read from UML failed, n = %d, errno = %d\n", n, 
	    errno);
    return(1);
  }
  if(output) write_output(fd, output);
  return(0);
}

struct addr_change_v4 {
  enum change_type what;
  unsigned char addr[4];
  unsigned char netmask[4];
};

static int control_v4(int fd, char *dev, struct output *output)
{
  struct addr_change_v4 change;
  int n;

  n = read(fd, &change, sizeof(change));
  if(n == sizeof(change)) 
    address_change(change.what, change.addr, dev, change.netmask, output);
  else if(n == 0) return(1);
  else {
    fprintf(stderr, "read from UML failed, n = %d, errno = %d\n", n, 
	    errno);
    return(1);
  }
  if(output) write_output(fd, output);
  return(0);
}

void ethertap_v0(int argc, char **argv)
{
  char *dev = argv[0];
  int fd = atoi(argv[1]);
  char *gate_addr = NULL;
  char *remote_addr = NULL;

  if(argc > 2){
    gate_addr = argv[2];
    remote_addr = argv[3];
  }
  ethertap(dev, fd, -1, gate_addr, remote_addr, 0, NULL);
}

void ethertap_v1_v2(int argc, char **argv)
{
  char *dev = argv[0];
  int data_fd = atoi(argv[1]);
  int control_fd = atoi(argv[2]);
  char *gate_addr = NULL;

  if(argc > 3) gate_addr = argv[3];
  ethertap(dev, data_fd, control_fd, gate_addr, NULL, 0, control_v1_v3);
}

void ethertap_v3(int argc, char **argv)
{
  char *dev = argv[0];
  int data_fd = atoi(argv[1]);
  char *gate = argv[2];

  ethertap(dev, data_fd, 1, gate, NULL, 1, control_v1_v3);
}

void ethertap_v4(int argc, char **argv)
{
  char *dev, *gate;
  int data_fd;
  struct output output = INIT_OUTPUT;

  if(argc < 2){
    add_output(&output, "uml_net : Too few arguments to ethertap_v4\n", -1);
    output_fail(&output, 1);
  }
  dev = argv[0];
  data_fd = atoi(argv[1]);
  gate = argc >= 3 ? argv[2] : NULL;
  ethertap(dev, data_fd, 1, gate, NULL, 1, control_v4);
}
