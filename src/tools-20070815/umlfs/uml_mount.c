#include <fuse/fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

static int init_fuse(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	char *mountpoint;

	if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) == -1)
		return -EINVAL;
        return fuse_mount(mountpoint, &args);
}

int main(int argc, char **argv)
{
	char *umid, fd_arg[sizeof("1234567890\0")];
	int fd;

	umid = argv[1];
	argv[1] = argv[0];
	argv++;
	argc--;
	fd = init_fuse(argc, argv);
	if(fd < 0)
		return 1;

	snprintf(fd_arg, sizeof(fd_arg), "%d", fd);
	execlp("uml_mconsole", "uml_mconsole", umid, "umlfs",  fd_arg, NULL);

	perror("Failed to exec uml_mconsole\n");
	
	return 1;
}
