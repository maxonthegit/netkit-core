TUNTAP = $(shell [ -e /usr/include/linux/if_tun.h ] && echo -DTUNTAP)

BIN = uml_net
CFLAGS = -g -Wall $(TUNTAP)

BIN_DIR ?= /usr/bin

OBJS = ethertap.o host.o output.o slip.o uml_net.o

ifneq ($(TUNTAP),)
	OBJS += tuntap.o
endif

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS)

clean : 
	rm -f $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(BIN_DIR)
	install -s -m 04755 $(BIN) $(DESTDIR)$(BIN_DIR)
