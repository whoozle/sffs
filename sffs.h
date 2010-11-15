#ifndef SFFS_MAIN_STRUCT_H__
#define SFFS_MAIN_STRUCT_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#ifdef LOG_STUBS
#	include <stdio.h>
#	define LOG_DEBUG(fmt) printf fmt; fputc('\n', stdout)
#	define LOG_INFO(fmt) printf fmt; fputc('\n', stdout)
#	define LOG_ERROR(fmt) printf fmt; fputc('\n', stdout)
#endif

#ifdef __cplusplus
extern "C" {
#endif 

typedef ssize_t (*write_func)(const void *ptr, size_t size);
typedef ssize_t (*read_func)(void *ptr, size_t size);
typedef off_t (*seek_func)(off_t offset, int whence);

struct sffs_block {
	off_t begin;
	off_t end;
	time_t mtime;
};

struct sffs_entry {
	char *name;
	size_t size;
	uint8_t padding;
	struct sffs_block block;
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
int sffs_unlink(struct sffs *fs, const char *fname);
int sffs_stat(struct sffs *fs, const char *fname, struct stat *buf);
const char* sffs_filename(struct sffs *fs, size_t index);

#ifdef __cplusplus
}
#endif 

#endif

