CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu99

all: sffs-tool

sffs-tool: sffs.o main.o
	$(CC) -o $@ $^

clean:
		rm -f *.o sffs-tool