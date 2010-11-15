CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb

all: sffs-tool

sffs-tool: sffs.o main.o
	$(CC) -o $@ $^

clean:
		rm -f *.o sffs-tool