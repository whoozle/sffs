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

int main(int argc, char **argv) {
  struct yffs fs;
  if (argc < 3  || argc > 4) {
    printf("Usage: yffs-add fs.img filename folder\n");
    return 0;
  }
  
  fs.write = fs_write_func;
  fs.read = fs_read_func;
  fs.seek = fs_seek_func;
  if (mount_image(&fs, argv[1]) == -1)
    return 2;
	
  int src_fd = -1;
  off_t src_size = 0;
  void *src_data = 0;
  char * filename = strdup(argv[2]);
  //printf("reading source %s...\n", filename);
  src_fd = open(filename, O_RDONLY);
  if (src_fd == -1) {
    //printf("file doesnt exist, creating it...\n");
    if((src_fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC)) == -1)
      perror("problem creating file\n");
    close(src_fd);
    if ((src_fd = open(filename, O_RDONLY)) == -1)
      perror("issue opening file...\n");
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
  
  if( argc == 3 ){
	  	printf("Writing file %s to folder %s\n", filename, argv[3]);   
  } else { // argc == 4 
	printf("Writing file %s\n", filename);   
  }  
  //**encrypt src_data**
  if (yffs_write(&fs, filename, src_data, src_size) == -1)
    goto next;
#if 0
  memset(src_data, '@', src_size);
  yffs_read(&fs, argv[f], src_data, src_size);
  fwrite(src_data, 1, src_size, stdout);
#endif
 next:
  free(src_data);
  close(src_fd);
  remove(filename);
		
  //printf("unmounting...\n");
  yffs_umount(&fs);

  close(fd);
	
  return 0;
}


