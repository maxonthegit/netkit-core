#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include "um_eth.h"

int packet_output(int in,struct  msghdr *msg) {
  struct connection_data *conn_in,*conn_out;
  int result = 0;
  int count = 0;
  int out;

  conn_in = uml_connection[in];

  if(debug > 1) {
    fprintf(stderr,"%02d %02d ->",in,conn_in->net_num);
    dump_packet(msg->msg_iov[1].iov_base,msg->msg_iov[1].iov_len,0);
    fprintf(stderr," ->");
  }

  for(out=0;out<=high_fd;out++) {
    conn_out = uml_connection[out];
    if(conn_out == NULL) continue;
    if(conn_out->stype == SOCKET_LISTEN) continue;

    if((out != in) && (conn_out->net_num == conn_in->net_num)) {

      if(debug > 1) {
        fprintf(stderr," %02d",out);
      }

      switch(conn_out->stype) {
        case SOCKET_CONNECTION:
        {
          int unsent = 0;
          int size = 0;
          int dumb = sizeof(size);

          if(ioctl(out,TIOCOUTQ,&unsent) ||
             getsockopt(out,SOL_SOCKET,SO_SNDBUF,&size,&dumb)) {
            CLOSE_CONNECTION(out);
            continue;
          }

          if((size - unsent) <
            (msg->msg_iov[0].iov_len+msg->msg_iov[1].iov_len)) {
            fprintf(stderr,"%d full! packet dropped\n",out);
            continue;
          }

send_again:
          result = sendmsg(out,msg,0);
          if(result < 0) {
            if(errno == EINTR || errno == EAGAIN) {
              CLOSE_CONNECTION(out);
              goto send_again;
            }
          } else if(result != (msg->msg_iov[0].iov_len +
            msg->msg_iov[1].iov_len)) {
            if(!result) {
              fprintf(stderr,"sendmsg zero\n");
              goto send_again;
            } else {
              fprintf(stderr,"sendmsg short write\n");
            }
          }
          break;
        }
        case SOCKET_LISTEN:
        default:
          continue;
      }
      count++;
    }
  }
  if(debug > 1) fprintf(stderr,"\n");
  return count;
}
