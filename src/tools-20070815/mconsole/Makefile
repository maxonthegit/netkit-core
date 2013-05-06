BIN = uml_mconsole
OBJS = $(BIN).o
CFLAGS ?= -g -Wall
LIBS = ../lib/libuml.a -lreadline -lncurses

BIN_DIR ?= /usr/bin

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean : 
	rm -f $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(BIN_DIR)
	install -s $(BIN) $(DESTDIR)$(BIN_DIR)
