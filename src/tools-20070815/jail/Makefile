OBJS = jail_uml.o
BIN = jail_uml
CFLAGS ?= -g -Wall

SBIN_DIR ?= /usr/sbin

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS)

install : $(BIN)
	install -d $(DESTDIR)$(SBIN_DIR)
	install -s $(BIN) $(DESTDIR)$(SBIN_DIR)

clean :
	rm -rf *~ $(BIN) $(OBJS) cell[0-9]* core* tty_log_cell*
