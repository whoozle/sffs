CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x -w

all: yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write oclean

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


# Use install command to send binarys to /usr/local/bin
# Requires SU
install: yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write
	mv yffs-ls /usr/local/bin
	mv yffs-cat /usr/local/bin
	mv yffs-create /usr/local/bin
	mv yffs-touch /usr/local/bin
	mv yffs-rm /usr/local/bin
	mv yffs-write /usr/local/bin
	rm -f *.o

clean:
	rm -f *.iso *.o yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

# Used so object files are never left behind
oclean:
	rm -f *.o

# Removes binarys from /usr/local/bin
uninstall:
	rm /usr/local/bin/yffs-ls
	rm /usr/local/bin/yffs-cat
	rm /usr/local/bin/yffs-create
	rm /usr/local/bin/yffs-touch
	rm /usr/local/bin/yffs-rm
	rm /usr/local/bin/yffs-write
