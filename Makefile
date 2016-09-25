CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x

all: yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

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
	rm *.o
#remove object files
	


# Use install command to send binarys to /usr/local/bin
# Requires SU
install:
	cp yffs-ls /usr/local/bin
	cp yffs-cat /usr/local/bin
	cp yffs-create /usr/local/bin
	cp yffs-touch /usr/local/bin
	cp yffs-rm /usr/local/bin
	cp yffs-write /usr/local/bin
	.PHONY install
clean:
	rm -f *.iso *.o yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

# Removes binarys from /usr/local/bin
uninstall:
	rm /usr/local/bin/yffs-ls
	rm /usr/local/bin/yffs-cat
	rm /usr/local/bin/yffs-create
	rm /usr/local/bin/yffs-touch
	rm /usr/local/bin/yffs-rm
	rm /usr/local/bin/yffs-write
	.PHONY uninstall
