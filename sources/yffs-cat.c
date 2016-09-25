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

    read:     ./sffs-tool fsname.img read test.txt
	
*/
int main(int argc, char **argv) {
	struct sffs fs;
	if (argc < 3) {
		printf("\tread:     ./sffs-tool fsname.img read test.txt\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

//READ FROM FILESYSTEM	
		int f;

		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(f = 2; f < argc; ++f) {
			struct stat buf;
			const char *fname = argv[f];
			void *src;
			ssize_t r;
			if (sffs_stat(&fs, fname, &buf) == 1)
				continue;
			printf("%s = %zu\n", fname, buf.st_size);
			src = malloc(buf.st_size);
			if (!src)
				return 1;
			
			r = sffs_read(&fs, fname, src, buf.st_size);
			if (r < 0)
				return 1;
			fwrite(src, 1, r, stdout);
			free(src);
		}
		sffs_umount(&fs);
	
	return 0;
}