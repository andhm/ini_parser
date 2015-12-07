CC = gcc
CFLAGS = -I../
LFLAGS = -L../ -liniparser

all: parse_file parse_string

parse_file: parse_file.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

parse_string: parse_string.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm -f parse_file parse_string
