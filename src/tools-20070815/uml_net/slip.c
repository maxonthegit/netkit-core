/* Copyright 2001, 2002 Jeff Dike and others
 * Licensed under the GPL
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include "host.h"
#include "output.h"

static void slip_up(int fd, char *gate_addr, char *remote_addr, 
		    char *netmask, struct output *output)
{
  char slip_name[sizeof("slxxxx\0")];
  char *up_argv[] = { "ifconfig", slip_name, gate_addr, "mtu", "1500", "up", 
		      NULL };
  int disc, sencap, n;
  
  disc = N_SLIP;
  if((n = ioctl(fd, TIOCSETD, &disc)) < 0){
    output_errno(output, "Setting slip line discipline");
    write_output(1, output);
    exit(1);
  }
  sencap = 0;
  if(ioctl(fd, SIOCSIFENCAP, &sencap) < 0){
    output_errno(output, "Setting slip encapsulation");
    write_output(1, output);
    exit(1);
  }
  snprintf(slip_name, sizeof(slip_name) - 1, "sl%d", n);
  if(do_exec(up_argv, 1, output)){
    write_output(1, output);
    exit(1);
  }
  forward_ip(output);
  if(remote_addr != NULL)
    route_and_arp(slip_name, remote_addr, netmask, 0, output);
}

static void slip_down(char *dev, char *remote_addr, char *netmask,
		      struct output *output)
{
  char *down_argv[] = { "ifconfig", dev, "0.0.0.0", "down", NULL };

  if(remote_addr != NULL)
    no_route_and_arp(dev, remote_addr, netmask, output);
  if(do_exec(down_argv, 1, output)){
    write_output(1, output);
    exit(1);
  }
}

static void slip_name(int fd, char *name, struct output *output)
{
  if(ioctl(fd, SIOCGIFNAME, name) < 0){
    output_errno(output, "Getting slip line discipline");
    write_output(1, output);
    exit(1);
  }
}

void slip_v0_v2(int argc, char **argv)
{
  char *op = argv[0], dev[sizeof("slnnnnn\0")];

  if(setreuid(0, 0) < 0){
    perror("slip - setreuid failed");
    exit(1);
  }
  
  if(!strcmp(op, "up")) 
    slip_up(atoi(argv[1]), argv[2], argv[3], NULL, NULL);
  else if(!strcmp(op, "down")){
    slip_name(atoi(argv[1]), dev, NULL);
    slip_down(dev, argv[2], NULL, NULL);
  }
  else {
    printf("slip - Unknown op '%s'\n", op);
    exit(1);
  }
}

void slip_v3(int argc, char **argv)
{
  struct output output = INIT_OUTPUT;
  char *op = argv[0], dev[sizeof("slnnnnn\0")];

  if(setreuid(0, 0) < 0){
    output_errno(&output, "slip - setreuid failed");
    exit(1);
  }

  if(!strcmp(op, "up")) 
    slip_up(atoi(argv[1]), argv[2], argv[3], NULL, &output);
  else if(!strcmp(op, "down")){
    slip_name(atoi(argv[1]), dev, &output);
    slip_down(dev, argv[2], NULL, &output);
  }
  else {
    printf("slip - Unknown op '%s'\n", op);
    exit(1);
  }
  write_output(1, &output);
}

void slip_v4(int argc, char **argv)
{
  struct output output = INIT_OUTPUT;
  char *op, dev[sizeof("slnnnnn\0")];

  if(setreuid(0, 0) < 0){
    output_errno(&output, "slip - setreuid failed");
    exit(1);
  }

  if(argc < 1){
    add_output(&output, "uml_net : too few arguments to slip_v4\n", -1);
    write_output(1, &output);
    exit(1);
  }

  op = argv[0];
  if(argc < 2){
      add_output(&output, "uml_net : too few arguments to slip_v4\n", -1);
      write_output(1, &output);
      exit(1);
  }
    
  if(!strcmp(op, "up")){
    slip_up(0, argv[1], NULL, NULL, &output);
  }
  else if(!strcmp(op, "down")){
    /* We now pass the interface to the "down" command as an open FD, rather
     * than as a name, because the latter allowed anybody to use uml_net to down
     * a interface he didn't own.
     *
     * Instead, as Steve Schmidtke said, an open FD is a proof that you own the
     * interface, so this way we should be safe. */
    slip_name(0, dev, &output);
    slip_down(dev, NULL, NULL, &output);
  }
  else {
    printf("slip - Unknown op '%s'\n", op);
    exit(1);
  }
  write_output(1, &output);
}

