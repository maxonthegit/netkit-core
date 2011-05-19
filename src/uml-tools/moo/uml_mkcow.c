#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cow.h"

void usage(const char progname[])
{
	fprintf(stderr, "%s usage: \n", progname);
	fprintf(stderr, "\t%s [-f] <COW file> <backing file>\n", progname);
	fprintf(stderr, "\t-f forces overwrite of an existing COW file\n");
	exit(2);
}

int main(int argc, char *argv[]) 
{
  int fd; 
  char *cow_file;
  char *backing_file;
  int bitmap_offset_out;
  unsigned long bitmap_len_out;
  int data_offset_out;
  int flags;
  mode_t mode;
  char *progname;
  
  progname = argv[0];
  argv++;
  argc--;

  if(argc < 1)
        usage(progname);

  flags = O_RDWR | O_CREAT | O_LARGEFILE;
  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  if(strcmp("-f", argv[0]) != 0){
	/* not forced, creating as exclusive */
	flags |= O_EXCL;
  }
  else {
  	argv++;
  	argc--;
  }
  
  if (argc < 2)
	usage(progname);
  
  cow_file = argv[0];
  backing_file = argv[1];
  
  if((fd = open(cow_file, flags, mode)) == -1) {
	perror("open COW file");
	exit(1);
  }
  
  if(init_cow_file(fd, cow_file, backing_file, 512, sysconf(_SC_PAGESIZE), 
		   &bitmap_offset_out, &bitmap_len_out, &data_offset_out)){
	perror("write_cow_header");
	exit(1);
  }

  return (0);
}

/* vim: ts=4 tw=74 
 */
