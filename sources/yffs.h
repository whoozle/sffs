#ifndef yffs_MAIN_STRUCT_H__
#define yffs_MAIN_STRUCT_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

//Allows program to run on macs
#ifdef __APPLE__
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#else
#include <endian.h>
#endif



#ifdef LOG_STUBS
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



struct yffs_block {
	off_t begin;
	off_t end;
	uint32_t mtime;
};

struct yffs_entry {
  char *name;
  size_t size;
  char *owner;
  char *dir; 
  unsigned int permBits;
  uint8_t padding;
  struct yffs_block block;
};

/*
contains the pointer to the space with the yffs_entry array.
ptr is like |<---yffs_entry--->|<---yffs_entry--->|<---yffs_entry--->|
*/
struct yffs_vector {
	uint8_t *ptr;
	size_t size;
};

struct yffs {
	size_t device_size; //Total filesystem size
	struct yffs_vector files, free; // 'files' = allocated vector, 'free' = free vector
	write_func write;
	read_func read;
	seek_func seek;
};

int yffs_format(struct yffs *fs);
int yffs_mount(struct yffs *fs);
int yffs_umount(struct yffs *fs);

ssize_t yffs_write(struct yffs *fs, const char *fname, const void *data, size_t size);
ssize_t yffs_read(struct yffs *fs, const char * directory, const char *fname, void *data, size_t size);
int yffs_unlink(struct yffs *fs, const char *fname, int recursive);
int yffs_stat(struct yffs *fs, const char *fname, struct stat *buf);
const char* yffs_filename(struct yffs *fs, size_t index, char * directory);
size_t yffs_get_largest_free(struct yffs *fs);
size_t yffs_get_total_free(struct yffs *fs);

void encrypt_file1(char * ftxt, int mode);
void decrypt_file1(char * ftxt, int mode);
void encrypt_file2(char * ftxt, int mode);
void decrypt_file2(char * ftxt, int mode);
void hash_filename(char * fname, int mode);
void sling_filename(char * fname, int mode);

char * substring(char *string, int position, int length);
size_t strlstchar(const char *str, const char ch);

#ifdef __cplusplus
}
#endif 

#endif

