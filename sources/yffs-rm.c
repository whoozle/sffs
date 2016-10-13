/*Team 22*/

#include "yffs.h"
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
	if (argc < 3) {
		printf("Usage:\n\n"
			   "\tcreatefs: ./yffs-tool fsname.img createfs 10000\n"
			   "\twrite:    ./yffs-tool fsname.img write test.txt\n"
			   "\tread:     ./yffs-tool fsname.img read test.txt\n"
			   "\tremove:	  ./yffs-tool fsname.img remove test.txt\n\n"
		       "\tlist:     ./yffs-tool fsname.img list\n"  
			   "\ttest:	  ./yffs-tool fsname.img test\n"
			   "\twear:	  ./yffs-tool fsname.img wear\n\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

//CREATE FILESYSTEM
	if (strcmp(argv[2], "createfs") == 0) {
		if (argc < 4) {
			printf("usage: 	createfs: ./yffs-tool fsname.img createfs 10000	\n");
			return 0;
		}
		fs.device_size = atoi(argv[3]);
		if (fs.device_size < 32) {
			printf("size must be greater than 32 bytes\n");
			return 1;
		}
		//printf("!~creating filesystem... (size: %u)\n", (unsigned)fs.device_size);
		{
			fd = open(argv[1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd == -1) {
				perror("open");
				return 1;
			}
			yffs_format(&fs);
			close(fd);
			
			if (truncate(argv[1], fs.device_size) == -1)
				perror("truncate");
		}
		return 0;
	} 

//WRITE TO FILESYSTEM
	 else if (strcmp(argv[2], "write") == 0) {
		int f;
	
		if (argc < 4) {
			printf("usage:	write: ./yffs-tool fsname.img write test.txt  \n");
			return 0;
		}
		
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			int src_fd = -1;
			off_t src_size = 0;
			void *src_data = 0;
			
			//printf("reading source %s...\n", argv[f]);
			if ((src_fd = open(argv[f], O_RDONLY)) == -1) {
				perror("open");
				continue;
			}
			src_size = lseek(src_fd, 0, SEEK_END);
			if (src_size == (off_t) -1) {
				perror("lseek");
				goto next;
			}
			src_data = malloc(src_size);
			if (src_data == NULL) {
				perror("malloc");
				goto next;
			}
	
			lseek(src_fd, 0, SEEK_SET);
			if (read(src_fd, src_data, src_size) != src_size) {
				perror("short read");
				goto next;
			}
			close(src_fd);
			//printf("writing file %s\n", argv[f]);
			
			if (yffs_write(&fs, argv[f], src_data, src_size) == -1)
				goto next;
#if 0
			memset(src_data, '@', src_size);
			yffs_read(&fs, argv[f], src_data, src_size);
			fwrite(src_data, 1, src_size, stdout);
#endif
		next:
			free(src_data);
			close(src_fd);
		}
		
		//printf("unmounting...\n");
		yffs_umount(&fs);
		
		close(fd);
	} 
	
//READ FROM FILESYSTEM	
	else if (strcmp(argv[2], "read") == 0) {
		int f;
		if (argc < 4) {
			printf("usage: read:  ./yffs-tool fsname.img read test.txt  \n");
			return 0;
		}
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			struct stat buf;
			const char *fname = argv[f];
			void *src;
			ssize_t r;
			if (yffs_stat(&fs, fname, &buf) == 1)
				continue;
			printf("%s = %zu\n", fname, buf.st_size);
			src = malloc(buf.st_size);
			if (!src)
				return 1;
			
			r = yffs_read(&fs, fname, src, buf.st_size);
			if (r < 0)
				return 1;
			fwrite(src, 1, r, stdout);
			free(src);
		}
		yffs_umount(&fs);
	} 	

//LIST OBJECTS IN FILESYSTEM
	else if (strcmp(argv[2], "list") == 0) {
		const char *name;
		size_t i, total, max;

		if (argc < 3) {
			printf("usage: 	list:  ./yffs-tool fsname.img list  \n");
			return 0;
		}

		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(i = 0; (name = yffs_filename(&fs, i)) != 0; ++i) {
			printf("%s\n", name);
		}
		
		max = yffs_get_largest_free(&fs);
		total = yffs_get_total_free(&fs);
		//printf("Free blocks, total: %zu, largest: %zu\n", total, max);

		yffs_umount(&fs);
	}

//REMOVE ITEM FROM FILESYSTEM	
	else if (strcmp(argv[2], "remove") == 0) {
		int f;
		if (argc < 4) {
			printf("usage: remove:  ./yffs-tool fsname.img remove test.txt  \n");
			return 0;
		}
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			yffs_unlink(&fs, argv[f]);
		}

		yffs_umount(&fs);
	} 

//TEST THE FILESYSTEM
else if (strcmp(argv[2], "test") == 0) {
		char buf[120];
		int i;
		for(i = 0; i < 120; ++i) 
			buf[i] = i;
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		yffs_write(&fs, "f1", buf, 4);
		yffs_write(&fs, "f2", buf, 4);
		yffs_write(&fs, "f3", buf, 4);
		yffs_write(&fs, "f1", buf, 4);
		yffs_write(&fs, "f2", buf, 4);
		yffs_write(&fs, "f3", buf, 30);
		yffs_write(&fs, "f1", buf, 4);
		yffs_write(&fs, "f2", buf, 4);
		yffs_write(&fs, "f3", buf, 60);
		yffs_write(&fs, "f1", buf, 4);
		yffs_write(&fs, "f2", buf, 4);
		yffs_write(&fs, "f3", buf, 90);
		yffs_write(&fs, "f1", buf, 4);
		yffs_write(&fs, "f2", buf, 4);
		yffs_write(&fs, "f3", buf, 120);
		
		yffs_umount(&fs);
	} 

//CHECK FILESYSTEM WEAR
	else if (strcmp(argv[2], "wear") == 0) {
		char buf[0x100];
		size_t i;
		unsigned long total = 0;
		for(i = 0; i < sizeof(buf); ++i) 
			buf[i] = i;

		fs.read = emu_read_func;
		fs.write = emu_write_func;
		fs.seek = emu_seek_func;
		fs.device_size = EMU_DEVICE_SIZE;
		if (yffs_format(&fs) == -1)
			return 2;

		if (yffs_mount(&fs) == -1)
			return 2;
		
		for(i = 0; i < EMU_DEVICE_SIZE; ++i) {
			size_t fsize = rand() & 0xff;
			char name[2] = {'a' + (rand() % 26), 0};
			if (yffs_write(&fs, name, buf, fsize) == -1) 
				break;
		}
		
		for(i = 0; i < EMU_DEVICE_SIZE; ++i) {
			uint32_t hits = emu_device_stat[i];
			//printf("%u;\n", hits);
			total += hits;
		}
		fprintf(stderr, ";total %lu -> ~%g writes average\n", total, 1.0f * total / EMU_DEVICE_SIZE);

		yffs_umount(&fs);
		
	} 

//HANDLES BAD INPUT
	else {
		printf("Usage:\n\n\tcreatefs: ./yffs-tool fsname.img createfs 10000\n"
			   "\twrite:    ./yffs-tool fsname.img write test.txt\n"
			   "\tread:     ./yffs-tool fsname.img read test.txt\n"
			   "\tremove:	  ./yffs-tool fsname.img remove test.txt\n\n"
		       "\tlist:     ./yffs-tool fsname.img list\n"  
			   "\ttest:	  ./yffs-tool fsname.img test\n"
			   "\twear:	  ./yffs-tool fsname.img wear\n\n");	
	}
	
	return 0;
}




//EMU
static ssize_t emu_write_func(const void *ptr, size_t size) {
	assert(emu_device_pos + size <= EMU_DEVICE_SIZE);
	for(size_t i = 0; i < size; ++i)
		++emu_device_stat[emu_device_pos + i];
	memcpy(emu_device + emu_device_pos, ptr, size);
	emu_device_pos += size;
	return size;
}

static ssize_t emu_read_func(void *ptr, size_t size) {
	assert(emu_device_pos + size <= EMU_DEVICE_SIZE);
	memcpy(ptr, emu_device + emu_device_pos, size);
	emu_device_pos += size;
	return size;
}

static off_t emu_seek_func(off_t offset, int whence) {
	switch(whence) {
	case SEEK_SET:
		emu_device_pos = offset;
		break;
	case SEEK_CUR:
		emu_device_pos += offset;
		break;
	case SEEK_END:
		emu_device_pos = EMU_DEVICE_SIZE + offset;
		break;
	default:
		assert(0);
	}
	assert(emu_device_pos <= EMU_DEVICE_SIZE);
	return emu_device_pos;
}
