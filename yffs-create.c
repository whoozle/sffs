/*Team 22*/

#include "sffs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>


//Used during 'wear' test
#define EMU_DEVICE_SIZE (0x10000)
static int fd;
static uint8_t emu_device[EMU_DEVICE_SIZE];
static unsigned emu_device_stat[EMU_DEVICE_SIZE];
static unsigned emu_device_pos;
static ssize_t emu_write_func(const void *ptr, size_t size); 
static ssize_t emu_read_func(void *ptr, size_t size); 
static off_t emu_seek_func(off_t offset, int whence); 


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

	createfs: ./sffs-tool fsname.img createfs 10000
	write:    ./sffs-tool fsname.img write test.txt
    read:     ./sffs-tool fsname.img read test.txt
	remove:	  ./sffs-tool fsname.img remove test.txt

	list:     ./sffs-tool fsname.img list  
	test:	  ./sffs-tool fsname.img test
	wear:	  ./sffs-tool fsname.img wear

*/
int main(int argc, char **argv) {
	struct sffs fs;
	if (argc < 3) {
		printf("Usage:\n\n"
			   "\tcreatefs: ./sffs-tool fsname.img createfs 10000\n"
			   "\twrite:    ./sffs-tool fsname.img write test.txt\n"
			   "\tread:     ./sffs-tool fsname.img read test.txt\n"
			   "\tremove:	  ./sffs-tool fsname.img remove test.txt\n\n"
		       "\tlist:     ./sffs-tool fsname.img list\n"  
			   "\ttest:	  ./sffs-tool fsname.img test\n"
			   "\twear:	  ./sffs-tool fsname.img wear\n\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

//CREATE FILESYSTEM
	if (strcmp(argv[2], "createfs") == 0) {
		if (argc < 4) {
			printf("usage: 	createfs: ./sffs-tool fsname.img createfs 10000	\n");
			return 0;
		}
		fs.device_size = atoi(argv[3]);
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
	}

