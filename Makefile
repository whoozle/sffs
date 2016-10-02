CFLAGS= -DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x -w

all: 	yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write oclean
	
yffs-ls: sources/yffs.o sources/yffs-ls.o
	@$(CC) -o bin/yffs-ls $^

yffs-cat: sources/yffs.o sources/yffs-cat.o
	@$(CC) -o bin/yffs-cat $^

yffs-create: sources/yffs.o sources/yffs-create.o
	@$(CC) -o bin/yffs-create $^

yffs-touch: sources/yffs.o sources/yffs-touch.o
	@$(CC) -o bin/yffs-touch $^ 

yffs-rm: sources/yffs.o sources/yffs-rm.o
	@$(CC) -o bin/yffs-rm $^

yffs-write: sources/yffs.o sources/yffs-write.o
	@$(CC) -o bin/yffs-write $^	


# Use install command to send binarys to /usr/local/bin
# Requires SU
install: yffs-ls yffs-cat yffs-create yffs-touch yffs-rm yffs-write
	@mv bin/yffs-ls /usr/local/bin
	@mv bin/yffs-cat /usr/local/bin
	@mv bin/yffs-create /usr/local/bin
	@mv bin/yffs-touch /usr/local/bin
	@mv bin/yffs-rm /usr/local/bin
	@mv bin/yffs-write /usr/local/bin
	@rm -f sources/*.o

clean:
	@rm -f *.iso sources/*.o bin/yffs-ls bin/yffs-cat bin/yffs-create bin/yffs-touch bin/yffs-rm bin/yffs-write

# Used so object files are never left behind
oclean:
	@rm -f sources/*.o
	
# Removes binarys from /usr/local/bin
uninstall:
	@rm -f /usr/local/bin/yffs-ls
	@rm -f /usr/local/bin/yffs-cat
	@rm -f /usr/local/bin/yffs-create
	@rm -f /usr/local/bin/yffs-touch
	@rm -f /usr/local/bin/yffs-rm
	@rm -f /usr/local/bin/yffs-write
