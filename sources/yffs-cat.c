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

    read:     ./yffs-cat fsname.img test.txt
	
*/
int main(int argc, char **argv) {
  struct yffs fs;
  if (argc < 3) {
    printf("tread:     ./yffs-cat fsname.img test.txt\n");
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
    if (yffs_stat(&fs, fname, &buf) == 1)
      continue;
    //printf("%s = %zu\n", fname, buf.st_size);
    src = malloc(buf.st_size);
    if (!src)
      return 1;
    /*fname is filename that needs to be "decrypted"
    in reality if you encrypt fname the same way it'll work 
    encrypt(fname, n) where n is encryption mode
    if you choose incorrect n the file will not be found 
    */
    r = yffs_read(&fs, fname, src, buf.st_size);
    if (r < 0)
      return 1;
    fwrite(src, 1, r, stdout);
    free(src);
  }
  yffs_umount(&fs);
	
  return 0;
}
