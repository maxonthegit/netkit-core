/* Copyright 2001 Jeff Dike and others
 * Licensed under the GPL
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <linux/if_tun.h>
#include "host.h"
#include "output.h"

static int do_tuntap_up(char *gate_addr, struct output *output, 
			struct ifreq *ifr)
{
  int tap_fd;
  char *ifconfig_argv[] = { "ifconfig", ifr->ifr_name, gate_addr, "netmask", 
			    "255.255.255.255", "up", NULL };
  char *insmod_argv[] = { "modprobe", "tun", NULL };

  if(setreuid(0, 0) < 0){
    output_errno(output, "setreuid to root failed : ");
    return(-1);
  }

  /* XXX hardcoded dir name and perms */
  umask(0);
  mkdir("/dev/net", 0755);

  /* #includes for MISC_MAJOR and TUN_MINOR bomb in userspace */
  mk_node("/dev/net/tun", 10, 200);

  do_exec(insmod_argv, 0, output);
  
  if((tap_fd = open("/dev/net/tun", O_RDWR)) < 0){
    output_errno(output, "Opening /dev/net/tun failed : ");
    return(-1);
  }
  memset(ifr, 0, sizeof(*ifr));
  ifr->ifr_flags = IFF_TAP | IFF_NO_PI;
  ifr->ifr_name[0] = '\0';
  if(ioctl(tap_fd, TUNSETIFF, (void *) ifr) < 0){
    output_errno(output, "TUNSETIFF : ");
    return(-1);
  }

  if((*gate_addr != '\0') && do_exec(ifconfig_argv, 1, output))
    return(-1);
  forward_ip(output);
  return(tap_fd);
}

static void tuntap_up(int fd, int argc, char **argv)
{
  struct ifreq ifr;
  struct msghdr msg;
  struct cmsghdr *cmsg;
  struct output output = INIT_OUTPUT;
  struct iovec iov[2];
  int tap_fd, *fd_ptr;
  char anc[CMSG_SPACE(sizeof(tap_fd))];
  char *gate_addr;

  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  iov[0].iov_base = NULL;
  iov[0].iov_len = 0;

  if(argc < 1){
    add_output(&output, "Too few arguments to tuntap_up\n", -1);
    goto out;
  }

  gate_addr = argv[0];
  if((tap_fd = do_tuntap_up(gate_addr, &output, &ifr)) > 0){
    iov[0].iov_base = ifr.ifr_name;
    iov[0].iov_len = IFNAMSIZ;

    msg.msg_control = anc;
    msg.msg_controllen = sizeof(anc);
  
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(tap_fd));

    msg.msg_controllen = cmsg->cmsg_len;

    fd_ptr = (int *) CMSG_DATA(cmsg);
    *fd_ptr = tap_fd;
  }

 out:
  iov[1].iov_base = output.buffer;
  iov[1].iov_len = output.used;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = iov;
  msg.msg_iovlen = sizeof(iov)/sizeof(iov[0]);
  msg.msg_flags = 0;

  if(sendmsg(fd, &msg, 0) < 0){
    perror("sendmsg");
    exit(1);
  }
}

void tuntap_v2(int argc, char **argv)
{
  char *op = argv[0];

  if(!strcmp(op, "up")) tuntap_up(atoi(argv[2]), argc - 3, argv + 3);
  else if(!strcmp(op, "add") || !strcmp(op, "del"))
    change_addr(argv[1], argv[2], argv[3], NULL, NULL);
  else {
    fprintf(stderr, "Bad tuntap op : '%s'\n", op);
    exit(1);
  }
}

void tuntap_v3(int argc, char **argv)
{
  char *op = argv[0];
  struct output output = INIT_OUTPUT;

  if(!strcmp(op, "up")) tuntap_up(1, argc - 2, argv + 2);
  else if(!strcmp(op, "add") || !strcmp(op, "del"))
    change_addr(op, argv[1], argv[2], NULL, &output);
  else {
    fprintf(stderr, "Bad tuntap op : '%s'\n", op);
    exit(1);
  }
  write_output(1, &output);
}

void tuntap_v4(int argc, char **argv)
{
  char *op;
  struct output output = INIT_OUTPUT;

  if(argc < 1){
    add_output(&output, "uml_net : too few arguments to tuntap_v4\n", -1);
    write_output(1, &output);
    exit(1);
  }
  op = argv[0];
  if(!strcmp(op, "up")) tuntap_up(1, argc - 1, argv + 1);
  else if(!strcmp(op, "add") || !strcmp(op, "del")){
    if(argc < 4)
      add_output(&output, "uml_net : too few arguments to tuntap "
		 "change_addr\n", -1);
    else change_addr(op, argv[1], argv[2], argv[3], &output);
    write_output(1, &output);
  }
  else {
    fprintf(stderr, "Bad tuntap op : '%s'\n", op);
    exit(1);
  }
}

