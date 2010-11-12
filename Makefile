CFLAGS=-Wall -pedantic -O -ggdb

all: sffs-tool

sffs-tool: sffs.o main.o
	$(CC) -o $@ $^

clean:
		rm -f *.o sffs-tool