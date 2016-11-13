CFLAGS= -DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x -w  

all: 	yffs-ls yffs-cat yffs-create yffs-rm yffs-edit yffs-add yffs-chown oclean

yffs-ls: sources/yffs.o sources/yffs-ls.o
	@mkdir -p bin
	@$(CC) -o bin/yffs-ls $^

yffs-cat: sources/yffs.o sources/yffs-cat.o
	@$(CC) -o bin/yffs-cat $^

yffs-create: sources/yffs-create.o
	@$(CC) -o bin/yffs-create $^

yffs-rm: sources/yffs.o sources/yffs-rm.o
	@$(CC) -o bin/yffs-rm $^

yffs-edit: sources/yffs.o sources/yffs-edit.o
	@$(CC) -o bin/yffs-edit $^
	
yffs-add: sources/yffs.o sources/yffs-add.o
	@$(CC) -o bin/yffs-add $^	

yffs-chown: sources/yffs.o sources/yffs-chown.o
	@$(CC) -o bin/yffs-chown $^	



# Use install command to send binarys to /usr/local/bin
# Requires SU
install: yffs-ls yffs-cat yffs-create yffs-add yffs-rm yffs-edit yffs-chown
	@mv bin/yffs-ls /usr/local/bin
	@mv bin/yffs-cat /usr/local/bin
	@mv bin/yffs-create /usr/local/bin
	@mv bin/yffs-rm /usr/local/bin
	@mv bin/yffs-edit /usr/local/bin
	@mv bin/yffs-add /usr/local/bin
	@mv bin/yffs-chown /usr/local/bin
	@rm -f sources/*.o

clean:
	@rm -f *.iso sources/*.o bin/*

# Used so object files are never left behind
oclean:
	@rm -f sources/*.o
	
# Removes binarys from /usr/local/bin
uninstall:
	@rm -f /usr/local/bin/yffs-ls
	@rm -f /usr/local/bin/yffs-cat
	@rm -f /usr/local/bin/yffs-create
	@rm -f /usr/local/bin/yffs-rm
	@rm -f /usr/local/bin/yffs-add
	@rm -f /usr/local/bin/yffs-edit
	@rm -f /usr/local/bin/yffs-chown
