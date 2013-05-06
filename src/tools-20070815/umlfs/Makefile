BIN = uml_mount
OBJS = $(BIN).o
# I have the FUSE headers in /usr/local/include
CFLAGS = -g -Wall -D_FILE_OFFSET_BITS=64 -I/usr/local/include
LIBS = -lfuse

BIN_DIR ?= /usr/bin

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean : 
	rm -f $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(BIN_DIR)
	install -s $(BIN) $(DESTDIR)$(BIN_DIR)
