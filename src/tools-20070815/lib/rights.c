#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>

/* pass the fd over an fd that is already connected to a socket */
int send_fd(int fd, int target, struct sockaddr *to, int to_len, void *msg,
	    int msg_len)
{
	/* File descriptors are specific to a process and normally only
	   sharable with another process by inheritence with fork(). The 
	   alternative is to use sendmsg with a special flag that says 
	   "I'm knowingly giving another process information from my private 
	   file descriptor table" (SCM_RIGHTS) 
	*/

	char anc[CMSG_SPACE(sizeof(fd))];
	struct msghdr hdr;
	struct cmsghdr *cmsg;
	struct iovec iov;
	int *fd_ptr;

	hdr.msg_name = to;
	hdr.msg_namelen = to_len;
	iov = ((struct iovec) { iov_base : msg,
			        iov_len :  msg_len });
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;
	hdr.msg_flags = 0;
	hdr.msg_control = anc;
	hdr.msg_controllen = sizeof(anc);

	cmsg = CMSG_FIRSTHDR(&hdr);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

	fd_ptr = (int *) CMSG_DATA(cmsg);
	*fd_ptr = fd;

	hdr.msg_controllen = cmsg->cmsg_len;

	if(sendmsg(target, &hdr, 0) < 0)
		return -errno;
	return 0;
}
