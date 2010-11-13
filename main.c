#include "sffs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

int main(int argc, char **argv) {
	struct sffs fs;
	if (argc <= 1) {
		printf("usage: [createfs image size(2^size)|write file|read file]\n");
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

	if (strcmp(argv[1], "createfs") == 0) {
		if (argc < 4) {
			printf("usage: createfs filename number\n");
			return 0;
		}
		fs.device_size = atoi(argv[3]);
		if (fs.device_size < 10) {
			printf("size must be greater than 10 (1024 bytes)\n");
			return 1;
		}
		fs.device_size = 1 << fs.device_size;
		printf("creating filesystem... (size: %u)\n", (unsigned)fs.device_size);
		{
			fd = open(argv[2], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd == -1) {
				perror("open");
				return 1;
			}
			sffs_format(&fs);
			close(fd);
			
			if (truncate(argv[2], fs.device_size) == -1)
				perror("truncate");
		}
		return 0;
	} else if (strcmp(argv[1], "list") == 0) {
		if (argc < 3) {
			printf("usage: list imagefile\n");
			return 0;
		}
		if (mount_image(&fs, argv[2]) == -1)
			return 2;

		sffs_umount(&fs);
	} else if (strcmp(argv[1], "write") == 0) {
		int f;
	
		if (argc < 4) {
			printf("usage: write imagefile file\n");
			return 0;
		}
		
		if (mount_image(&fs, argv[2]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			int src_fd = -1;
			off_t src_size = 0;
			void *src_data = 0;
			
			printf("reading source %s...\n", argv[f]);
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
			printf("writing file %s\n", argv[f]);
			
			sffs_write(&fs, argv[f], src_data, src_size);
		
		next:
			free(src_data);
			close(src_fd);
		}
		
		printf("unmounting...\n");
		sffs_umount(&fs);
		
		close(fd);
	} else if (strcmp(argv[1], "read") == 0) {
		int f;
		if (argc < 4) {
			printf("usage: read imagefile file\n");
			return 0;
		}
		if (mount_image(&fs, argv[2]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			struct stat buf;
			const char *fname = argv[f];
			if (sffs_stat(&fs, fname, &buf) == 1)
				continue;
			printf("%s = %zu\n", fname, buf.st_size);
			void *src = malloc(buf.st_size);
			if (!src)
				return 1;
			ssize_t r = sffs_read(&fs, fname, src, buf.st_size);
			if (r < 0)
				return 1;
			fwrite(src, 1, r, stdout);
			free(src);
		}
		sffs_umount(&fs);
	} else if (strcmp(argv[1], "remove") == 0) {
		int f;
		if (argc < 4) {
			printf("usage: remove imagefile file\n");
			return 0;
		}
		if (mount_image(&fs, argv[2]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			sffs_unlink(&fs, argv[f]);
		}

		sffs_umount(&fs);
	} else {
		printf("unknown command: %s\n", argv[1]);
	}
	
	return 0;
}
