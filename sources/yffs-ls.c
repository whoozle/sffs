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
static pthread_mutex_t mutex;



static ssize_t fs_write_func(const void *ptr, size_t size) {
	pthread_mutex_lock(&mutex);
	int wr = write(fd, ptr, size);
	pthread_mutex_unlock(&mutex);

	return wr; 
}

static ssize_t fs_read_func(void *ptr, size_t size) {
	pthread_mutex_lock(&mutex);
	int rd = read(fd, ptr, size);
	pthread_mutex_unlock(&mutex);

	return rd; 
}

static off_t fs_seek_func(off_t offset, int whence) {
	pthread_mutex_lock(&mutex);
	int lsk = lseek(fd, offset, whence);
	pthread_mutex_unlock(&mutex);

	return lsk; 
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

	list:     ./yffs-tool fsname.img list  

*/
int main(int argc, char **argv) {
  struct yffs fs;
  if (argc < 2) {
    printf("usage: 	list:  ./yffs-ls fsname.img\n");     //hopefully eventually: "./yffs-ls fsname.img /path/to/dir"
    return 0;
  }

  fs.write = fs_write_func;
  fs.read = fs_read_func;
  fs.seek = fs_seek_func;

  //LIST OBJECTS IN FILESYSTEM
  //const char *name;
  char * name;
  size_t i, total, max;

  if (mount_image(&fs, argv[1]) == -1)
    return 2;

  for(i = 0; (name = yffs_filename(&fs, i)) != 0; ++i) {
	  /*
	  name needs to be actually decrypted
	  decrypt(name, n) where n is decrytpion method
	  */
      decrypt(name, 0);
    printf("%s\n", name);
  }
		
  //max = yffs_get_largest_free(&fs);
  //total = yffs_get_total_free(&fs);
  //printf("Free blocks, total: %zu, largest: %zu\n", total, max);

  yffs_umount(&fs);
  return 0;
}
