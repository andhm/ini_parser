CC = gcc
CFLAGS = -fPIC -Wall
AR = ar

SO_TARGET = libiniparser.so
SRCS = ini_parser.c
OBJS = $(SRCS:.c=.o)

all: libiniparser.a

.c.o:
	$(CC) $(CFLAGS) -c $<


libiniparser.a: $(OBJS)
	$(AR) -r $@ $^

clean:
	rm -rf $(OBJS)

.PHONY: example
example : libiniparser.a
	@(cd example; $(MAKE))

