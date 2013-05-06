/* Copyright 2001 Jeff Dike
 * Licensed under the GPL
 */

#ifndef __HOST_H
#define __HOST_H

#include "output.h"

enum change_type { ADD_ADDR, DEL_ADDR };

extern int do_exec(char **args, int need_zero, struct output *output);
extern int route_and_arp(char *dev, char *addr, char *netmask, int need_route, 
			 struct output *output);
extern int no_route_and_arp(char *dev, char *addr, char *netmask, 
			    struct output *output);
extern void forward_ip(struct output *output);

extern void address_change(enum change_type what, unsigned char *addr_str, 
			   char *dev, unsigned char *netmask, 
			   struct output *output);
extern int mk_node(char *devname, int major, int minor);
extern void change_addr(char *op, char *dev, char *address, char *netmask,
			struct output *output);
extern int is_a_device(char *dev);

#endif
