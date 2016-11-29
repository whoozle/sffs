CFLAGS= -DLOG_STUBS -Wall -pedantic -ggdb -std=gnu9x -w  

all: 	yffs-ls yffs-cat yffs-create yffs-rm yffs-edit yffs-add yffs-chown yffs-chmod oclean

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

yffs-chmod: sources/yffs.o sources/yffs-chmod.o
	@$(CC) -o bin/yffs-chmod $^	


# Use install command to send binarys to /usr/local/bin
# Requires SU
install: yffs-ls yffs-cat yffs-create yffs-add yffs-rm yffs-edit yffs-chown yffs-chmod
	@cp bin/yffs-ls /usr/local/bin
	@cp bin/yffs-cat /usr/local/bin
	@cp bin/yffs-create /usr/local/bin
	@cp bin/yffs-rm /usr/local/bin
	@cp bin/yffs-edit /usr/local/bin
	@cp bin/yffs-add /usr/local/bin
	@cp bin/yffs-chown /usr/local/bin
	@cp bin/yffs-chmod /usr/local/bin
	@cp project_resources/mans/yffs.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-cat.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-chown.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-edit.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-rm.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-add.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-chmod.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-create.1 /usr/share/man/man1/
	@cp project_resources/mans/yffs-ls.1 /usr/share/man/man1/
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
	@rm -f /usr/local/bit/yffs-chmod
