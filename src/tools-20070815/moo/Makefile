BIN = uml_moo uml_mkcow

uml_moo_OBJS = uml_moo.o cow.o
uml_mkcow_OBJS = uml_mkcow.o cow.o

CFLAGS ?= -g -Wall
override CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_GNU_SOURCE

BIN_DIR ?= /usr/bin

all : $(BIN)

uml_moo : $(uml_moo_OBJS)
	$(CC) $(CFLAGS) -o $@ $($@_OBJS)

uml_mkcow : $(uml_mkcow_OBJS)
	$(CC) $(CFLAGS) -o $@ $($@_OBJS)

$(uml_moo_OBJS) $(uml_mkcow_OBJS) : cow.h cow_sys.h

clean : 
	rm -f $(foreach bin,$(BIN),$($(bin)_OBJS)) $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(BIN_DIR)
	install -s $(BIN) $(DESTDIR)$(BIN_DIR)
