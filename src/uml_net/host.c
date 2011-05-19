/* Copyright 2001 Jeff Dike and others
 * Licensed under the GPL
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "output.h"
#include "host.h"

int do_exec(char **args, int need_zero, struct output *output)
{
  int pid, status, fds[2], n;
  char buf[256], **arg;

  if(output){
    add_output(output, "*", -1);
    for(arg = args; *arg; arg++){
      add_output(output, " ", -1);
      add_output(output, *arg, -1);
    }
    add_output(output, "\n", -1);
    if(pipe(fds) < 0){
      perror("Creating pipe");
      output = NULL;
    }
  }
  if((pid = fork()) == 0){
    if(output){
      close(fds[0]);
      if((dup2(fds[1], 1) < 0) || (dup2(fds[1], 2) < 0)) perror("dup2");
    }
    execvp(args[0], args);
    fprintf(stderr, "Failed to exec '%s'", args[0]);
    perror("");
    exit(1);
  }
  else if(pid < 0){
    output_errno(output, "fork failed");
    return(-1);
  }
  if(output){
    close(fds[1]);
    while((n = read(fds[0], buf, sizeof(buf))) > 0) add_output(output, buf, n);
    if(n < 0) output_errno(output, "Reading command output");
  }
  if(waitpid(pid, &status, 0) < 0){
    output_errno(output, "waitpid");
    return(-1);
  }
  if(need_zero && (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))){
    sprintf(buf, "'%s' didn't exit with status 0\n", args[0]);
    add_output(output, buf, -1);
    return(-1);
  }
  return(0);
}

static void local_net_do(char **argv, int if_index, char *addr_str, 
			 char *netmask_str, char *skip_dev,
			 struct output *output)
{
  FILE *fp;
  unsigned long addr, netmask, if_addr;
  int a[4], n[4];
  char buf[1024], iface[32], out[1024];

  if(sscanf(addr_str, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) != 4){
    add_output(output, "local_net_do didn't parse address", -1);
    return;
  }
  addr = (a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
  if(sscanf(netmask_str, "%d.%d.%d.%d", &n[0], &n[1], &n[2], &n[3]) != 4){
    add_output(output, "local_net_do didn't parse netmask", -1);
    return;
  }
  netmask = (n[3] << 24) | (n[2] << 16) | (n[1] << 8) | n[0];
  argv[if_index] = iface;
  if((fp = fopen("/proc/net/route", "r")) == NULL){
    output_errno(output, "Couldn't open /proc/net/route");
    return;
  }
  if(fgets(buf, sizeof(buf), fp) == NULL) return;
  buf[0] = '\0';
  while(fgets(buf, sizeof(buf), fp) != NULL){
    int n;
    n = sscanf(buf, "%32s %lx %*x %*x %*x %*x %*x", iface, &if_addr);
    if(n != 2){
      sprintf(out, "Didn't parse /proc/net/route, got %d\n on '%s'", n, buf);
      add_output(output, out, -1);
      return;
    }
    if(strcmp(skip_dev, iface) && ((addr & netmask) == (if_addr & netmask)))
      do_exec(argv, 0, output);
    buf[0] = '\0';
  }
  fclose(fp);
}

int is_a_device(char *dev)
{
  int i;

  for(i = 0; i < strlen(dev); i++){
    if(!isalnum(dev[i])) return(0);
  }
  return(1);
}

int route_and_arp(char *dev, char *addr, char *netmask, int need_route,
		  struct output *output)
{
  char echo[sizeof("echo 1 > /proc/sys/net/ipv4/conf/XXXXXXXXX/proxy_arp")];
  char *echo_argv[] = { "bash", "-c", echo, NULL };
  char *route_argv[] = { "route", "add", "-host", addr, "dev", dev, NULL };
  char *arp_argv[] = { "arp", "-Ds", addr, "eth0",  "pub", NULL };

  if(!is_a_device(dev)){
    add_output(output, "Device doesn't contain only alphanumeric characters\n",
	       -1);
    return(-1);
  }
  if(do_exec(route_argv, need_route, output)) return(-1);
  snprintf(echo, sizeof(echo) - 1, 
	   "echo 1 > /proc/sys/net/ipv4/conf/%.9s/proxy_arp", dev);
  do_exec(echo_argv, 0, output);
  if(netmask) local_net_do(arp_argv, 3, addr, netmask, dev, output);
  else do_exec(arp_argv, 0, output);
  return(0);
}

int no_route_and_arp(char *dev, char *addr, char *netmask, 
		     struct output *output)
{
  char echo[sizeof("echo 0 > /proc/sys/net/ipv4/conf/XXXXXXXXX/proxy_arp")];
  char *no_echo_argv[] = { "bash", "-c", echo, NULL };
  char *no_route_argv[] = { "route", "del", "-host", addr, "dev", dev, NULL };
  char *no_arp_argv[] = { "arp", "-i", "eth0", "-d", addr, "pub", NULL };

  if(!is_a_device(dev)){
    add_output(output, "Device doesn't contain only alphanumeric characters\n",
	       -1);
    return(-1);
  }
  do_exec(no_route_argv, 0, output);
  snprintf(echo, sizeof(echo) - 1, 
	   "echo 0 > /proc/sys/net/ipv4/conf/%.9s/proxy_arp", dev);
  do_exec(no_echo_argv, 0, output);
  if(netmask) local_net_do(no_arp_argv, 2, addr, netmask, dev, output);
  else do_exec(no_arp_argv, 0, output);
  return(0);
}

void forward_ip(struct output *output)
{
  char *forw_argv[] = { "bash",  "-c", 
			"echo 1 > /proc/sys/net/ipv4/ip_forward", NULL };
  do_exec(forw_argv, 0, output);
}

void address_change(enum change_type what, unsigned char *addr_str, char *dev, 
		    unsigned char *netmask_str, struct output *output)
{
  char addr[sizeof("255.255.255.255\0")];
  char netmask[sizeof("255.255.255.255\0")], *n = NULL;

  snprintf(addr, sizeof(addr) - 1, "%d.%d.%d.%d", addr_str[0], addr_str[1], 
	   addr_str[2], addr_str[3]);
  if(netmask_str != NULL){
    snprintf(netmask, sizeof(netmask) - 1, "%d.%d.%d.%d", netmask_str[0], 
	     netmask_str[1], netmask_str[2], netmask_str[3]);
    n = netmask;
  }
  switch(what){
  case ADD_ADDR:
    route_and_arp(dev, addr, n, 0, output);
    break;
  case DEL_ADDR:
    no_route_and_arp(dev, addr, n, output);
    break;
  default:
    fprintf(stderr, "address_change - bad op : %d\n", what);
    return;
  }
}

/* This is a routine to do a 'mknod' on the /dev/tap<n> if possible:
 * Return: 0 is ok, -1=already open, etc.
 */

int mk_node(char *devname, int major, int minor)
{
  struct stat statval;
  int retval;

  /* first do a stat on the node to see whether it exists and we
   * had some other reason to fail:
   */
  retval = stat(devname, &statval);
  if(retval == 0) return(0);
  else if(errno != ENOENT){
    /* it does exist. We are just going to return -1, 'cause there
     * was some other problem in the open :-(.
     */
    return -1;
  }

  /* It doesn't exist. We can create it. */

  return(mknod(devname, S_IFCHR|S_IREAD|S_IWRITE, makedev(major, minor)));
}

void add_address_v4(int argc, char **argv)
{
  struct output output = INIT_OUTPUT;
  
  if(setreuid(0, 0) < 0){
    output_errno(&output, "setreuid");
    exit(1);
  }
  route_and_arp(argv[0], argv[1], argv[2], 0, &output);
  write_output(1, &output);
}

void del_address_v4(int argc, char **argv)
{
  struct output output = INIT_OUTPUT;
  
  if(setreuid(0, 0) < 0){
    output_errno(&output, "setreuid");
    exit(1);
  }
  no_route_and_arp(argv[0], argv[1], argv[2], &output);
  write_output(1, &output);
}

void change_addr(char *op, char *dev, char *address, char *netmask,
		 struct output *output)
{
  if(setreuid(0, 0) < 0){
    output_errno(output, "setreuid");
    exit(1);
  }
  if(!strcmp(op, "add")) route_and_arp(dev, address, netmask, 0, output);
  else no_route_and_arp(dev, address, netmask, output);
}
