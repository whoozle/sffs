CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x

all: sffs-tool #yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

sffs-tool: sffs.o main.o
	$(CC) -o sffs-tool $^

yffs-ls: sffs.o yffs-ls.o
	$(CC) -o yffs-ls $^

yffs-cat: sffs.o yffs-cat.o
	$(CC) -o yffs-cat $^

yffs-create: sffs.o yffs-create.o
	$(CC) -o yffs-create $^

yffs-touch: sffs.o yffs-touch.o
	$(CC) -o yffs-touch $^

yffs-rm: sffs.o yffs-rm.o
	$(CC) -o yffs-rm $^

yffs-write: sffs.o yffs-write.o
	$(CC) -o yffs-write $^

clean:
		rm -f *.o sffs-tool
