#ifndef _UM_ETH_NET_H
#define _UM_ETH_NET_H

#define UM_ETH_NET_MAX_PACKET	1544
#define UM_ETH_NET_HDR_SIZE	(sizeof(unsigned int)*2)

#define UM_ETH_NET_PORT		5299
#define UM_ETH_NET_DEFAULT_NUM	100

enum packet_type {
	PACKET_DATA = 1,
	PACKET_MGMT
};

enum socket_type {
	SOCKET_LISTEN = 1,
	SOCKET_CONNECTION,
	SOCKET_PHY,
	SOCKET_TAP
};

struct connection_data {
	enum socket_type stype;
	int net_num;
        int too_little;
	int fd;
};

extern int packet_output(int, struct msghdr*);
extern int packet_input(struct connection_data*);
extern int socket_phy_setup(char *);
extern int socket_tap_setup(char *);
extern void dump_packet(unsigned char *buffer,int size,int term);

extern struct connection_data *uml_connection[];
extern int high_fd;
extern int debug;
extern fd_set perm;

#define NEW_CONNECTION          \
              (struct connection_data*)malloc(sizeof(struct connection_data))

#define CLOSE_CONNECTION(y)     \
        FD_CLR(y,&perm);        \
        fprintf(stderr,"socket close\n");\
        close(y);               \
        free(uml_connection[y]);\
        uml_connection[y] = NULL;

#define FILL_CONNECTION(y)                              \
        uml_connection[y]->stype = SOCKET_CONNECTION;   \
        uml_connection[y]->net_num = UM_ETH_NET_DEFAULT_NUM;    \
	uml_connection[y]->fd = y;			\
        if(y > high_fd) high_fd = y;                    \
        FD_SET(y,&perm);

#endif
