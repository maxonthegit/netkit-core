#ifndef __COW_SYS_H__
#define __COW_SYS_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

static inline void *cow_malloc(int size)
{
	return(malloc(size));
}

static inline void cow_free(void *ptr)
{
	free(ptr);
}

static inline int cow_printf(char *fmt, ...)
{
	va_list args;
	int n;

	va_start(args, fmt);
	n = vprintf(fmt, args);
	va_end(args);
	return(n);
}

static inline char *cow_strdup(char *str)
{
	return(strdup(str));
}

static inline int cow_seek_file(int fd, __u64 offset)
{
	__u64 actual;

	actual = lseek(fd, offset, SEEK_SET);
	if(actual != offset)
		return(-errno);
	return(0);
}

static inline int cow_file_size(char *file, long long *size_out)
{
	struct stat buf;

	if(stat(file, &buf) == -1){
		cow_printf("Couldn't stat \"%s\" : errno = %d\n", file, errno);
		return(-errno);
	}
	if(S_ISBLK(buf.st_mode)){
		int fd, blocks;

		if((fd = open(file, O_RDONLY)) < 0){
			cow_printf("Couldn't open \"%s\", errno = %d\n", file,
				   errno);
			return(-errno);
		}
		if(ioctl(fd, BLKGETSIZE, &blocks) < 0){
			cow_printf("Couldn't get the block size of \"%s\", "
				   "errno = %d\n", file, errno);
			close(fd);
			return(-errno);
		}
		*size_out = ((long long) blocks) * 512;
		close(fd);
		return(0);
	}
	*size_out = buf.st_size;
	return(0);
}

static inline int cow_write_file(int fd, char *buf, int size)
{
	return(write(fd, buf, size));
}

#endif

/*
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
