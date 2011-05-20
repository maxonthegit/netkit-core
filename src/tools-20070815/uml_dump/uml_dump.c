/*
 * Copyright 2008 
 * Authors:
 *   - Julien Iguchi-Cartigny
 *   - Jean-Baptiste Machemie
 *   - Benoit Bitarelle
 * Licensed under the GPL
 *
 * TODO: receive packets from multiple uml switchs
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "../uml_router/switch.h"
#include "../uml_router/uml_switch.h"
#include "../uml_router/port.h"

/***************** PCAP Part *****************/

int output_fd;

unsigned char pcap_file_header[] = {
  0xd4, 0xc3, 0xb2, 0Xa1, // magic number
  0x02, 0x00, 0x04, 0x00, // major and minor version number: 2.4
  0x00, 0x00, 0x00, 0x00, // thiszone: GMT
  0x00, 0x00, 0x00, 0x00, // accuracy: 0
  0xEA, 0x05, 0x00, 0x00, // max length of captured packet: 1514
  0x01, 0x00, 0x00, 0x00  // data link type: ethernet
};

unsigned int pcap_packet_header[] = {
  0, // timestamp second
  0, // timestamp microsecond
  0, // number of octets of packet saved in file
  0  // actual length of packet
};

struct timeval tv;

void dump_packet(struct packet *packet, int size) {

  int w;

  gettimeofday(&tv, (void*)0);

  pcap_packet_header[0] = tv.tv_sec;
  pcap_packet_header[1] = tv.tv_usec;
  pcap_packet_header[2] = size;
  pcap_packet_header[3] = size;

  w = fwrite(pcap_packet_header, sizeof(unsigned char), 
             sizeof(pcap_packet_header), stdout);

  if(w<=0)
    perror("error when writing packet header to stdout");
  
  w = fwrite((unsigned char*)packet, sizeof(unsigned char), size, stdout);

  if(w<=0)
    perror("error when writing packet data to stdout");

  w = fflush(stdout);

  if(w!=0)
    perror("error flushing stdout");
}

/***************** UML Part *****************/

static char *prog;

static char *ctl_socket = "/tmp/uml.ctl";

int connect_fd, data_out_fd, data_in_fd;

void cleanup() {
  close(connect_fd);
  close(data_out_fd);
}

static void sig_handler(int sig)
{
  fprintf(stderr,"Caught signal %d, cleaning up and exiting\n", sig);
  cleanup();
  signal(sig, SIG_DFL);
  kill(getpid(), sig);
}

static void usage(void)
{
  fprintf(stderr, "Usage : %s control-socket\n"
                  "if no parameter %s is used as control socket\n", 
                  prog, ctl_socket);
  exit(1);
}


int main(int argc, char *argv[]) {

  prog = argv[0];
  argv++;
  argc--;

  /* parameter -help */
  if(argc == 1) {

    if(!strcmp(argv[0], "-help")){
        usage();
    }

    ctl_socket = argv[0];
  }

  if(argc > 1) {
    usage();
  }

  /* ask for the data in socket */
  if((data_in_fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }

  if(fcntl(data_in_fd, F_SETFL, O_NONBLOCK) < 0){
    perror("setting O_NONBLOCK on data in fd");
    exit(1);
  }

  /* prepare the data in structure */
  struct sockaddr_un data_in_sun;  

  struct {
    char zero;
    int pid;
    int usecs;
  } name;

  struct timeval tv;

  name.zero = 0;
  name.pid = getpid();
  gettimeofday(&tv, NULL);
  name.usecs = tv.tv_usec;
 
  data_in_sun.sun_family = AF_UNIX;
  memcpy(data_in_sun.sun_path, &name, sizeof(name));

  /* bind data_in_socket */
  if(bind(data_in_fd, (struct sockaddr*) &data_in_sun, 
          sizeof(data_in_sun)) < 0){
    perror("binding to data in socket");
    exit(1);
  }

  /* ask for the connect socket */
  if((connect_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
    perror("socket connect");
    exit(1);
  }

  /* start the connect socket */
  struct sockaddr_un connect_sun;

  connect_sun.sun_family = AF_UNIX;
  strncpy(connect_sun.sun_path, ctl_socket, sizeof(connect_sun.sun_path));

  if(connect(connect_fd, (struct sockaddr *)&connect_sun, 
             sizeof(connect_sun)) < 0){
    perror("connect connect");
    fprintf(stderr,"check the path to control socket: %s\n", ctl_socket);
    exit(1);
  }

  /* prepare the request */
  struct request_v3 request;
  request.magic = 0xfeedface;
  request.version = 3;
  request.type = REQ_NEW_DUMP;
  request.sock = data_in_sun;

  int n;

  /* send the request */
  n = send(connect_fd, &request, sizeof(request), 0);

  if(n < sizeof(request)) {
    fprintf(stderr,"not enough byte sent or error: %d\n", n);
    exit(1);
  }

  /* receive the data out socket addr (we don't really care about it) */
  struct sockaddr_un data_out_sun;

  n = recv(connect_fd, &data_out_sun, sizeof(data_out_sun), 0); 

  if(n < sizeof(data_out_sun)) {
    fprintf(stderr,"not enough byte received or error: %d\n", n);
    exit(1);
  }

  /* prepare the pool */
  struct pollfd p[2];

  p[0].fd = connect_fd;
  p[0].events = POLLIN;
  p[1].fd = data_in_fd;
  p[1].events = POLLIN;

  /* setup the signal handler for CTRL-C */
  if(signal(SIGINT, sig_handler) < 0)
    perror("Setting handler for SIGINT");

  /* setup the signal handler for closed pipe by reader */
  if(signal(SIGPIPE, sig_handler) < 0)
    perror("Setting handler for SIGPIPE");

 /* writing pcap header to stdout */
  int w = fwrite(pcap_file_header, sizeof(unsigned char), 
                 sizeof(pcap_file_header), stdout);

  if(w<=0)
    perror("error when writing cap header to stdout");

  struct packet packet;

  /* the main loop */
  while(1) {

    /* waiting */
    n = poll(p, 2, -1);

    if(n < 0){
      if(errno == EINTR) continue; /* if poll stop by a signal */
      perror("poll");
      break;
    }

    /* check control socket */
    if(p[0].revents) {
      if(p[0].revents & POLLHUP){
	fprintf(stderr,"uml_switch disconnected\n");
	break;
      } else {
        fprintf(stderr,"control packet in excess ?\n");
        exit(1);
      }
    }

    /* check data out socket */
    if(p[1].revents) {

      n = recv(data_in_fd, &packet, sizeof(packet), 0); 
      if(n < 0 && errno != EAGAIN) {
        perror("recv data out socket\n");
        exit(1);
      }

      dump_packet(&packet,n);
    }
  }

  cleanup();

  return 0;
}
