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


static int mount_image(struct sffs *fs, const char *fname) {
	fd = open(fname, O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	return sffs_mount(fs);
}


/* SFFS 

	list:     ./sffs-tool fsname.img list  

*/
int main(int argc, char **argv) {
	struct sffs fs;
	if (argc < 2) {
		printf("\tlist:     ./sffs-tool fsname.img list\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

//LIST OBJECTS IN FILESYSTEM
		const char *name;
		size_t i, total, max;

		if (argc < 2) {
			printf("usage: 	list:  ./sffs-tool fsname.img list  \n");
			return 0;
		}

		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(i = 0; (name = sffs_filename(&fs, i)) != 0; ++i) {
			printf("%s\n", name);
		}
		
		max = sffs_get_largest_free(&fs);
		total = sffs_get_total_free(&fs);
		printf("Free blocks, total: %zu, largest: %zu\n", total, max);

		sffs_umount(&fs);
	return 0;
}
