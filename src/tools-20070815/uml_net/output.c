/* Copyright 2001 Jeff Dike and others
 * Licensed under the GPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "output.h"

void write_output(int fd, struct output *output)
{
  if(output == NULL) return;
  if(write(fd, &output->used, sizeof(output->used)) != sizeof(output->used)){
    perror("write_output : write of length failed");
    exit(1);
  }
  if(write(fd, output->buffer, output->used) != output->used){
    perror("write_output : write of data failed");
    exit(1);
  }
  output->used = 0;
}

void add_output(struct output *output, char *new, int len)
{
  int n, start;

  if(output == NULL){
    fprintf(stderr, "%s", new);
    return;
  }

  if(len == -1) len = strlen(new);
  n = len;
  if(output->used == 0) n++;
  if(output->used + n > output->total){
    output->total = output->used + n;
    output->total = (output->total + 4095) & ~4095;
    if(output->buffer == NULL){
      if((output->buffer = malloc(output->total)) == NULL){
	perror("mallocing new output buffer");
	*output = ((struct output) { 0, 0, NULL});
	return;
      }
    }
    else if((output->buffer = realloc(output->buffer, output->total)) == NULL){
	perror("reallocing new output buffer");
	*output = ((struct output) { 0, 0, NULL});
	return;
    }
  }
  if(output->used == 0) start = 0;
  else start = output->used - 1;
  strncpy(&output->buffer[start], new, len);
  output->used += n;
  output->buffer[output->used - 1] = '\0';
}

void output_errno(struct output *output, char *str)
{
  if(output == NULL) perror(str);
  else {
    add_output(output, str, -1);
    add_output(output, ": ", -1);
    add_output(output, strerror(errno), -1);
    add_output(output, "\n", -1);
  }
}

