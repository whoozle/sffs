/*Team 22*/

#include "yffs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>


static int fd;
static ssize_t fs_write_func(const void *ptr, size_t size) {
	return write(fd, ptr, size);
}

static ssize_t fs_read_func(void *ptr, size_t size) {
	return read(fd, ptr, size);
}

static off_t fs_seek_func(off_t offset, int whence) {
	return lseek(fd, offset, whence);
}


static int mount_image(struct yffs *fs, const char *fname) {
	fd = open(fname, O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	return yffs_mount(fs);
}

/* yffs 

	createfs: ./yffs-tool fsname.img createfs 10000
	write:    ./yffs-tool fsname.img write test.txt
    read:     ./yffs-tool fsname.img read test.txt
	remove:	  ./yffs-tool fsname.img remove test.txt

	list:     ./yffs-tool fsname.img list  
	test:	  ./yffs-tool fsname.img test
	wear:	  ./yffs-tool fsname.img wear

*/
int main(int argc, char **argv) {
	struct yffs fs;

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;


//REMOVE ITEM FROM FILESYSTEM	
	int f;
	if (argc < 3) {
		printf("usage: remove:  ./yffs-tool fsname.img test.txt  \n");
		return 0;
	}
	if (mount_image(&fs, argv[1]) == -1) {
		return 2;
	}

	for(f = 2; f < argc; ++f) {
		yffs_unlink(&fs, argv[f]);
	}

	
	yffs_umount(&fs);
} 




