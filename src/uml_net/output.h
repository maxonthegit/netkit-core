/* Copyright 2001 Jeff Dike and others
 * Licensed under the GPL
 */

#ifndef __OUTPUT_H
#define __OUTPUT_H

struct output {
  int total;
  int used;
  char *buffer;
};

#define INIT_OUTPUT { 0, 0, NULL }

extern void write_output(int fd, struct output *output);
extern void add_output(struct output *output, char *new, int len);
extern void output_errno(struct output *output, char *str);

#endif
