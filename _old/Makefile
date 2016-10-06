CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x

all: sffs-tool

sffs-tool: sffs.o main.o
	$(CC) -o sffs-tool $^

clean:
	rm -f *.o sffs-tool
