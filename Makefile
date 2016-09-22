CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x

all: sffs-tool #yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

sffs-tool: sffs.o main.o
	$(CC) -o sffs-tool $^

yffs-ls:

yffs-cat:

yffs-create:

yffs-touch:

yffs-rm:

yffs-write:

clean:
		rm -f *.o sffs-tool
