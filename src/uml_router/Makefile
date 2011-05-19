TUNTAP = $(shell [ -e /usr/include/linux/if_tun.h ] && echo -DTUNTAP)

OBJS = hash.o port.o uml_switch.o 
BIN = uml_switch
CFLAGS = -g -Wall $(TUNTAP)

BIN_DIR ?= /usr/bin

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
	install -s $(BIN) $(DESTDIR)$(BIN_DIR)
