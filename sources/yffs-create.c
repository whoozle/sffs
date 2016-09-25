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



/* YFFS 

	createfs: ./yffs-create fsname.img 10000

*/

int main(int argc, char **argv) {
	struct sffs fs;
	if (argc < 3) {
		printf("Usage:\n\n"
			   "\tcreatefs: ./yffs-create fsname.img 10000\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

		//CREATE FILESYSTEM
		if (argc < 3) {
			printf("usage: 	createfs: ./sffs-tool fsname.img createfs 10000	\n");
			return 0;
		}
		
		fs.device_size = atoi(argv[2]);
		if (fs.device_size < 32) {
			printf("size must be greater than 32 bytes\n");
			return 1;
		}
		
		printf("creating filesystem... (size: %u)\n", (unsigned)fs.device_size);
		{
			fd = open(argv[1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd == -1) {
				perror("open");
				return 1;
			}
			sffs_format(&fs);
			close(fd);
			
			if (truncate(argv[1], fs.device_size) == -1)
				perror("truncate");
		}
		return 0;
}

