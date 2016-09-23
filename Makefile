CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x

all: sffs-tool yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

sffs-tool: sffs.o main.o
	$(CC) -o sffs-tool $^

yffs-ls: yffs.o yffs-ls.o
	$(CC) -o yffs-ls $^

yffs-cat: yffs.o yffs-cat.o
	$(CC) -o yffs-cat $^

yffs-create: yffs.o yffs-create.o
	$(CC) -o yffs-create $^

yffs-touch: yffs.o yffs-touch.o
	$(CC) -o yffs-touch $^

yffs-rm: yffs.o yffs-rm.o
	$(CC) -o yffs-rm $^

yffs-write: yffs.o yffs-write.o
	$(CC) -o yffs-write $^

clean:
	rm -f *.o sffs-tool yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write
