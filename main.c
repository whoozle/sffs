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
	if (argc < 3) {
		printf("usage: image-file [createfs file size|write file|read file]\n");
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;

	if (strcmp(argv[2], "createfs") == 0) {
		if (argc < 4) {
			printf("usage: createfs filename size\n");
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
	} else if (strcmp(argv[2], "list") == 0) {
		const char *name;
		size_t i, total, max;

		if (argc < 3) {
			printf("usage: list imagefile\n");
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
	} else if (strcmp(argv[2], "write") == 0) {
		int f;
	
		if (argc < 4) {
			printf("usage: write imagefile file\n");
			return 0;
		}
		
		if (mount_image(&fs, argv[1]) == -1)
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
			
			if (sffs_write(&fs, argv[f], src_data, src_size) == -1)
				goto next;
#if 0
			memset(src_data, '@', src_size);
			sffs_read(&fs, argv[f], src_data, src_size);
			fwrite(src_data, 1, src_size, stdout);
#endif
		next:
			free(src_data);
			close(src_fd);
		}
		
		printf("unmounting...\n");
		sffs_umount(&fs);
		
		close(fd);
	} else if (strcmp(argv[2], "read") == 0) {
		int f;
		if (argc < 4) {
			printf("usage: read imagefile file\n");
			return 0;
		}
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
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
	} else if (strcmp(argv[2], "remove") == 0) {
		int f;
		if (argc < 4) {
			printf("usage: remove imagefile file\n");
			return 0;
		}
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		for(f = 3; f < argc; ++f) {
			sffs_unlink(&fs, argv[f]);
		}

		sffs_umount(&fs);
	} else if (strcmp(argv[2], "test") == 0) {
		char i, buf[120];
		for(i = 0; i < 120; ++i) 
			buf[i] = i;
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		sffs_write(&fs, "f1", buf, 4);
		sffs_write(&fs, "f2", buf, 4);
		sffs_write(&fs, "f3", buf, 4);
		sffs_write(&fs, "f1", buf, 4);
		sffs_write(&fs, "f2", buf, 4);
		sffs_write(&fs, "f3", buf, 30);
		sffs_write(&fs, "f1", buf, 4);
		sffs_write(&fs, "f2", buf, 4);
		sffs_write(&fs, "f3", buf, 60);
		sffs_write(&fs, "f1", buf, 4);
		sffs_write(&fs, "f2", buf, 4);
		sffs_write(&fs, "f3", buf, 90);
		sffs_write(&fs, "f1", buf, 4);
		sffs_write(&fs, "f2", buf, 4);
		sffs_write(&fs, "f3", buf, 120);
		
		sffs_umount(&fs);
	} else {
		printf("unknown command: %s\n", argv[2]);
	}
	
	return 0;
}
