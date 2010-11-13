#ifndef SFFS_MAIN_STRUCT_H__
#define SFFS_MAIN_STRUCT_H__

#include <sys/types.h>
#include <stdint.h>

typedef ssize_t (*write_func)(const void *ptr, size_t size);
typedef ssize_t (*read_func)(void *ptr, size_t size);
typedef off_t (*seek_func)(off_t offset, int whence);

struct sffs_area {
	off_t begin;
	off_t end;
};

struct sffs_entry {
	char *name;
	struct sffs_area area;
	struct sffs_entry *next;
};

struct sffs_vector {
	uint8_t *ptr;
	size_t size;
};

struct sffs {
	size_t device_size;
	struct sffs_vector files, free;

	write_func write;
	read_func read;
	seek_func seek;
};

int sffs_format(struct sffs *fs);
int sffs_mount(struct sffs *fs);
int sffs_umount(struct sffs *fs);
ssize_t sffs_write(struct sffs *fs, const char *fname, const void *data, size_t size);
ssize_t sffs_read(struct sffs *fs, const char *fname, void *data, size_t size);

#endif

