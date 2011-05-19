CC = gcc

CFLAGS = -Wall -ggdb

all: um_eth_serv um_eth_tool

SERV_OBJS = input.o um_eth_serv.o output.o

um_eth_serv: $(SERV_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERV_OBJS)

um_eth_tool: um_eth_tool.o
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf um_eth_serv um_eth_tool *.o core *~
