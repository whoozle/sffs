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
	//opens argv[1] which is the fs
	fd = open(fname, O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	//mounts fs, returns an int error/success code
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
	/*
	TO DO:
		Redo arguments to accept a flag (-a, -r) 
		Usage: ./yffs-write filesystem original new
		Rewrite code to append or rewrite file
	*/
	struct sffs fs;
	if (argc < 3) {
		printf("Usage:\n\n"
			   "\twrite:    ./yffs-write fsname.img newFile\n"
			"\twrite: 	./yffs-write fsname.img orignal new\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;


//WRITE TO FILESYSTEM
		int f = 2;
		
		//mount system named at argv[1], return if it failed
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		FILE *tmp;
		tmp = fopen("temp.txt", "w");

		if (argc == 4) {
			f = 3; 
			struct stat buf;
			const char *fname = argv[2];
			void *src;
			ssize_t r;
			sffs_stat(&fs, fname, &buf);
			printf("%s = %zu\n", fname, buf.st_size);
			src = malloc(buf.st_size);
			r = sffs_read(&fs, fname, src, buf.st_size);
			fwrite(src, 1, r, tmp);
			free(src);
		}
		
		//src_fd is the file descriptor for argv[f]
		int src_fd = -1;
			
		//offset
		off_t src_size = 0;
			
		//buffer
		void *src_data = 0;
			
		printf("reading source %s...\n", argv[f]);
			
		//set src_fd to arv[f] 
		if ((src_fd = open(argv[f], O_RDONLY)) == -1) {
			perror("open");
		}
			
		//set size
		src_size = lseek(src_fd, 0, SEEK_END);
	//	src_size = lseek(src_fd, 0, SEEK_END) + buf.st_size;
			
		//check if size is legitimate
		if (src_size == (off_t) -1) {
			perror("lseek");
			goto next;
		}
			
		//make space in memory for data
		src_data = malloc(src_size);
			
		//checks malloc
		if (src_data == NULL) {
			perror("malloc");
			goto next;
		}
	
		lseek(src_fd, 0, SEEK_SET);
			
		//reads from argv[f]->src_data
		if (read(src_fd, src_data, src_size) != src_size) {
			perror("short read");
			goto next;
		}
			
		//no longer need arv[f]
		close(src_fd);
		printf("writing file %s\n", argv[f]);
			
		//writes the file
		if (sffs_write(&fs, argv[f], src_data, src_size) == -1)
			goto next;
#if 0
			memset(src_data, '@', src_size);
			sffs_read(&fs, argv[f], src_data, src_size);
			fwrite(src_data, 1, src_size, stdout);
#endif
		/* 'next' appears to just close everything
			implying it is only called IF there is nothing to write, i.e. no more args
		*/
		next:
			free(src_data);
			close(src_fd);
		
		printf("unmounting...\n");
		sffs_umount(&fs);
		
		close(fd);
		return 0;
	} 
	
