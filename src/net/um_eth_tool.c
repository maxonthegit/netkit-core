#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(char *name) {
    fprintf(stderr,"\n%s -i ifname [-n netnum] [-a address] [-s script]\n\n",name);
    fprintf(stderr,"\taddress:\n");
    fprintf(stderr,"\t\tINET <ipaddress:port>\n");
    fprintf(stderr,"\t\tUNIX <absolut path>\n");
    fprintf(stderr,"\t\tTUNTAP <tape name>\n");
    fprintf(stderr,"\n\tnetnum:\n");
    fprintf(stderr,"\t\tonly has effect with INET and UNIX sockets\n");
    fprintf(stderr,"\n\tscript:\n");
    fprintf(stderr,"\t\tscript exec'ed by host after interface is connected\n");
}

int main(int argc,char **argv) {
  struct ifreq ifr;
  char *temp;
  char *ifname = NULL;
  int fd;
  int opt;

  memset(&ifr,0,sizeof(struct ifreq));

  if((fd = socket(PF_INET,SOCK_STREAM,0))<= 0) {
    perror("socket");
    exit(errno);
  }

  if(argc <= 1) usage(argv[0]);

  while((opt = getopt(argc,argv,"i:a:n:s:h")) != EOF) {
    switch(opt) {
      case 'i':
        ifname = optarg;
        strcpy(ifr.ifr_name,optarg);
        break;
      case 'a':
        if(!ifname) {
          usage(argv[0]);
          exit(-1);
        }
        if((temp = index(optarg,':'))) {
          struct sockaddr_in *sin = (struct sockaddr_in*)&ifr.ifr_dstaddr;
          char *ip_str = optarg;
          char *port_str = temp + 1;
          *temp = '\0';
  
          sin->sin_family = AF_INET;
          sin->sin_port = htons(atoi(port_str));
          sin->sin_addr.s_addr = inet_addr(ip_str);
  
          printf("%s will connect to AF_INET server at %s:%s\n",ifname,ip_str,
            port_str);
        } else if(argv[3][0] == '/') {
          struct sockaddr_un *sun = (struct sockaddr_un*)&ifr.ifr_dstaddr;
  
          sun->sun_family = AF_UNIX;
          strcpy(sun->sun_path,argv[3]);
          printf("%s will connect to AF_UNIX server at %s\n",ifname,optarg);
        } else {
          struct sockaddr_un *sun = (struct sockaddr_un*)&ifr.ifr_dstaddr;
  
          sun->sun_family = 0;
          strcpy(sun->sun_path,optarg);
          printf("%s will connect to TUNTAP interface %s\n",ifname,optarg);
        }
        if(ioctl(fd,SIOCDEVPRIVATE+1,&ifr)<0) {
          perror("ioctl");
          exit(errno);
        }
        break;
      case 's':
      {
        struct sockaddr_un *sun = (struct sockaddr_un*)&ifr.ifr_dstaddr;
  
        if(!ifname) {
          usage(argv[0]);
          exit(-1);
        }

        sun->sun_family = AF_UNIX;
        strcpy(sun->sun_path,optarg);
        printf("host will execute %s when %s is connected\n",optarg,ifname);

        if(ioctl(fd,SIOCDEVPRIVATE+2,&ifr)<0) {
          perror("ioctl");
          exit(errno);
        }
        break;
      }
      case 'n':
        if(!ifname) {
          usage(argv[0]);
          exit(-1);
        }
        ifr.ifr_flags = atoi(optarg);
        if(ioctl(fd,SIOCDEVPRIVATE,&ifr)<0) {
          perror("Net number");
          exit(errno);
        }
        break;
      case 'h':
      default:
        usage(argv[0]);
        exit(-1);
    }
  }



  return 0;
}
