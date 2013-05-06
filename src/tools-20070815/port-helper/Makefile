OBJS = port-helper.o
BIN = port-helper
CFLAGS ?= -g -Wall
LIBS = ../lib/libuml.a

LIB_DIR ?= /usr/lib/uml

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean : 
	rm -f $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(LIB_DIR)
	install -s $(BIN) $(DESTDIR)$(LIB_DIR)
