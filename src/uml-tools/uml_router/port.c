#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "switch.h"
#include "hash.h"
#include "port.h"

struct packet {
  struct {
    unsigned char dest[ETH_ALEN];
    unsigned char src[ETH_ALEN];
    unsigned char proto[2];
  } header;
  unsigned char data[1500];
};

struct port {
  struct port *next;
  struct port *prev;
  int control;
  void *data;
  int data_len;
  unsigned char src[ETH_ALEN];
  void (*sender)(int fd, void *packet, int len, void *data);
};

static struct port *head = NULL;

#define IS_BROADCAST(addr) ((addr[0] & 1) == 1)

static void free_port(struct port *port)
{
  if(port->prev) port->prev->next = port->next;
  else head = port->next;
  if(port->next) port->next->prev = port->prev;
  free(port);
}

void close_port(int fd)
{
  struct port *port;

  for(port = head; port != NULL; port = port->next){
    if(port->control == fd) break;
  }
  if(port == NULL){
    fprintf(stderr, "No port associated with descriptor %d\n", fd);
    return;
  }
  delete_hash(port->src);
  free_port(port);
}

static void update_src(struct port *port, struct packet *p)
{
  struct port *last;

  /* We don't like broadcast source addresses */
  if(IS_BROADCAST(p->header.src)) return;  

  last = find_in_hash(p->header.src);

  if(port != last){
    /* old value differs from actual input port */

    printf(" Addr: %02x:%02x:%02x:%02x:%02x:%02x New port %d",
	   p->header.src[0], p->header.src[1], p->header.src[2],
	   p->header.src[3], p->header.src[4], p->header.src[5],
	   port->control);

    if(last != NULL){
      printf(" old port %d", last->control);
      delete_hash(p->header.src);
    }
    printf("\n");

    memcpy(port->src, p->header.src, sizeof(port->src));
    insert_into_hash(p->header.src, port);
  }
  update_entry_time(p->header.src);
}

static void send_dst(struct port *port, struct packet *packet, int len, 
		     int hub)
{
  struct port *target, *p;

  target = find_in_hash(packet->header.dest);
  if((target == NULL) || IS_BROADCAST(packet->header.dest) || hub){
    if((target == NULL) && !IS_BROADCAST(packet->header.dest)){
      printf("unknown Addr: %02x:%02x:%02x:%02x:%02x:%02x from port ",
	     packet->header.src[0], packet->header.src[1], 
	     packet->header.src[2], packet->header.src[3], 
	     packet->header.src[4], packet->header.src[5]);
      if(port == NULL) printf("UNKNOWN\n");
      else printf("%d\n", port->control);
    } 

    /* no cache or broadcast/multicast == all ports */
    for(p = head; p != NULL; p = p->next){
      /* don't send it back the port it came in */
      if(p == port) continue;

      (*p->sender)(p->control, packet, len, p->data);
    }
  }
  else (*target->sender)(target->control, packet, len, target->data);
}

static void handle_data(int fd, int hub, struct packet *packet, int len, 
			void *data, int (*matcher)(int port_fd, int data_fd, 
						   void *port_data,
						   int port_data_len, 
						   void *data))
{
  struct port *p;

  for(p = head; p != NULL; p = p->next){
    if((*matcher)(p->control, fd, p->data, p->data_len, data)) break;
  }
  
  /* if we have an incoming port (we should) */
  if(p != NULL) update_src(p, packet);
  else printf("Unknown connection for packet, shouldn't happen.\n");

  send_dst(p, packet, len, hub);  
}

static int match_tap(int port_fd, int data_fd, void *port_data, 
		     int port_data_len, void *data)
{
  return(port_fd == data_fd);
}

void handle_tap_data(int fd, int hub)
{
  struct packet packet;
  int len;

  len = read(fd, &packet, sizeof(packet));
  if(len < 0){
    if(errno != EAGAIN) perror("Reading tap data");
    return;
  }
  handle_data(fd, hub, &packet, len, NULL, match_tap);
}

int setup_port(int fd, void (*sender)(int fd, void *packet, int len, 
				      void *data), void *data, int data_len)
{
  struct port *port;

  port = malloc(sizeof(struct port));
  if(port == NULL){
    perror("malloc");
    return(-1);
  }
  port->next = head;
  if(head) head->prev = port;
  port->prev = NULL;
  port->control = fd;
  port->data = data;
  port->data_len = data_len;
  port->sender = sender;
  head = port;
  printf("New connection\n");
  return(0);
}

struct sock_data {
  int fd;
  struct sockaddr_un sock;
};

static void send_sock(int fd, void *packet, int len, void *data)
{
  struct sock_data *mine = data;
  int err;
  
  err = sendto(mine->fd, packet, len, 0, (struct sockaddr *) &mine->sock,
	       sizeof(mine->sock));
  if(err != len){
    fprintf(stderr, "send_sock sending to fd %d ", mine->fd);
    perror("");
  }
}

static int match_sock(int port_fd, int data_fd, void *port_data, 
		      int port_data_len, void *data)
{
  struct sock_data *mine = data;
  struct sock_data *port = port_data;

  if(port_data_len != sizeof(*mine)) return(0);
  return(!memcmp(&port->sock, &mine->sock, sizeof(mine->sock)));
}

void handle_sock_data(int fd, int hub)
{
  struct packet packet;
  struct sock_data data;
  int len, socklen = sizeof(data.sock);

  len = recvfrom(fd, &packet, sizeof(packet), 0, 
		 (struct sockaddr *) &data.sock, &socklen);
  if(len < 0){
    if(errno != EAGAIN) perror("handle_sock_data");
    return;
  }
  data.fd = fd;

  handle_data(fd, hub, &packet, len, &data, match_sock);
}

int setup_sock_port(int fd, struct sockaddr_un *name, int data_fd)
{
  struct sock_data *data;

  data = malloc(sizeof(*data));
  if(data == NULL){
    perror("setup_sock_port");
    return(-1);
  }
  *data = ((struct sock_data) { fd : 	data_fd,
				sock :	*name });
  return(setup_port(fd, send_sock, data, sizeof(*data)));
}

static void service_port(struct port *port)
{
  int n;
  char c;

  n = read(port->control, &c, sizeof(c));
  if(n < 0) perror("Reading request");
  else if(n == 0) printf("Disconnect\n");
  else printf("Bad request\n");  
}

int handle_port(int fd)
{
  struct port *p;

  for(p = head; p != NULL; p = p->next){
    if(p->control == fd){
      service_port(p);
      return(0);
    }
  }
  return(1);
}

