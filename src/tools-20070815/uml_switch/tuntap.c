#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include "port.h"

static void send_tap(int fd, void *packet, int len, void *unused)
{
  int n;

  n = write(fd, packet, len);
  if(n != len){
    if(errno != EAGAIN) perror("send_tap");
  }
}

int open_tap(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if((fd = open("/dev/net/tun", O_RDWR)) < 0){
    perror("Failed to open /dev/net/tun");
    return(-1);
  }
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
  if(ioctl(fd, TUNSETIFF, (void *) &ifr) < 0){
    perror("TUNSETIFF failed");
    close(fd);
    return(-1);
  }
  err = setup_port(fd, send_tap, NULL, 0);
  if(err) return(err);
  return(fd);
}

