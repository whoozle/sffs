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
		printf("Usage: createfs: ./yffs-create fsname.img 10000\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

	fs.device_size = atoi(argv[2]);
	if (fs.device_size < 32) {
		printf("YFFS must be greater then 32 bytes!\n");
		return 1;
	}
	
	{
		fd = open(argv[1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR ); //Create a file'system' at this location, w/ permissions.
		if (fd == -1) {
			perror("open");
			return 1;
		}
		sffs_format(&fs); //This takes the new fs, and adds a 'first block' with an empty header.
		close(fd);
		
		if (truncate(argv[1], fs.device_size) == -1)
			perror("truncate");
	}
	//Only print output on errors
	//printf("YFFS creation successful!\n \'%s\' -> size: %u bytes\n",argv[1], (unsigned)fs.device_size);
	return 0;
}

