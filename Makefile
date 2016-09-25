CFLAGS=-DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x -w

all: 	yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write oclean
	
yffs-ls: sources/yffs.o sources/yffs-ls.o
	$(CC) -o yffs-ls $^

yffs-cat: sources/yffs.o sources/yffs-cat.o
	$(CC) -o yffs-cat $^

yffs-create: sources/yffs.o sources/yffs-create.o
	$(CC) -o yffs-create $^

yffs-touch: sources/yffs.o sources/yffs-touch.o
	$(CC) -o yffs-touch $^

yffs-rm: sources/yffs.o sources/yffs-rm.o
	$(CC) -o yffs-rm $^

yffs-write: sources/yffs.o sources/yffs-write.o
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
	rm -f sources/*.o

clean:
	rm -f *.iso sources/*.o yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write

# Used so object files are never left behind
oclean:
	rm -f sources/*.o
	
# Removes binarys from /usr/local/bin
uninstall:
	rm -f /usr/local/bin/yffs-ls
	rm -f /usr/local/bin/yffs-cat
	rm -f /usr/local/bin/yffs-create
	rm -f /usr/local/bin/yffs-touch
	rm -f /usr/local/bin/yffs-rm
	rm -f /usr/local/bin/yffs-write
