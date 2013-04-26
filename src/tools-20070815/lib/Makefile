LIB = libuml.a
OBJS = rights.o
CFLAGS ?= -g -Wall

all : $(LIB)

$(LIB) : $(OBJS)
	$(AR) r $@ $<

clean :
	rm -f $(LIB) $(OBJS)

install :
