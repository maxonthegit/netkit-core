#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include "um_eth.h"

static unsigned int header[2];
static int header_size = sizeof(header);

static unsigned char _buffer[10240];
static unsigned char *buffer = _buffer + 2;
static int buffer_size = sizeof(_buffer) - 2;

static struct msghdr msg;
static struct iovec io[2];

int packet_input(struct connection_data *conn_in) {
  struct sockaddr addr;
  int in = conn_in->fd;
  int new_fd;
  int len;
  int result = -1;
  int unread;

  io[0].iov_base = header;
  io[0].iov_len = header_size;

  io[1].iov_base = buffer;
  io[1].iov_len = buffer_size;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = io;
  msg.msg_iovlen = 2;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  switch(conn_in->stype) {
    case SOCKET_LISTEN:
    {
      if(debug > 3) fprintf(stderr,"connect\n");
      new_fd = accept(in,(struct sockaddr*)&addr,&len);
      fcntl(new_fd, F_SETFL, O_ASYNC | O_NONBLOCK);
      if((uml_connection[new_fd] = NEW_CONNECTION) == NULL) {
        perror("malloc of uml_connection data");
	return -1;
      }
      FILL_CONNECTION(new_fd);
      return 0;
    }
    case SOCKET_CONNECTION:
    {
      msg.msg_iovlen = 1;
      result = recvmsg(in,&msg,MSG_PEEK);
      msg.msg_iovlen = 2;

      if(result >= header_size) {

        if(ioctl(in,FIONREAD,&unread)) {
          CLOSE_CONNECTION(in);
          return -1;
        }
        if(unread < (ntohl(header[1]) + header_size)) {
          return 0;
        }
	io[1].iov_len = ntohl(header[1]);

        result = recvmsg(in,&msg,MSG_WAITALL);
        if(result != (io[0].iov_len + io[1].iov_len)) {
          fprintf(stderr,"Short read\n");
        }

	if(result > 0) {
	  conn_in->too_little = 0;
        }

      } else {

	if(++conn_in->too_little > 1000) {
          CLOSE_CONNECTION(in);
          return -1;
	}

      }
      break;
    }
    default:
      break;
  }				/* switch(conn_in->stype) */

  if(result <=0) {
    if((!result) || (errno != EAGAIN && errno != EINTR)) {
      CLOSE_CONNECTION(in);
      return -1;
    }
    return 0;
  }

  switch(ntohl(header[0])) {
    case PACKET_MGMT:
    {
      memcpy(&result,io[1].iov_base,sizeof(result));
      conn_in->net_num = ntohl(result);
      if(debug) fprintf(stderr,"mgmt: %d is now on %d\n",in,conn_in->net_num);
      break;
    }
    case PACKET_DATA:
    {
      if(debug > 3) {
        fprintf(stderr,"%02d %02d <- ",in,conn_in->net_num);
        dump_packet(io[1].iov_base,ntohl(header[1]),1);
      }
      packet_output(in,&msg);
      break;
    }
    default:
    {
      fprintf(stderr,"unknow packet type (%d)\n",ntohl(header[0]));
      return -1;
    }
  }				/* switch(ntohl(header[0])) */
  return 1;
}
