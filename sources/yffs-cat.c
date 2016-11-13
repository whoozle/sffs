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
  int mode = 0;
  if (argv[argc - 1][0] == '-') {
        mode = argv[argc - 1][1] - 48;
  }

  if (mount_image(&fs, argv[1]) == -1)
    return 2;

  for(f = 2; f < 3; ++f) {
    struct stat buf;
    char *fname = argv[f];
    void *src;
    ssize_t r;


    int index = strlstchar(fname, '/');
    char * directory = (char*)substring(fname, 0, index+1);

    if(index == -1){
      char * buff = "/";
      directory = buff;
    }

    char * filename;
    filename = (char *)substring(fname, index+1, strlen(fname) - (index+1));

    if(strcmp(filename, "") == 0)
    {
      //Only a folder was given
      LOG_ERROR(("YFFS: yffs-ls %s %s", argv[1], argv[2]));
      return 1;
    }

    encrypt_file(filename, mode);
    if (yffs_stat(&fs, filename, &buf) != -1){
      //Check to see if the file exists
    }
    else{
      LOG_ERROR(("File %s not found", filename));
      return 1;
    }

    //printf("%s = %zu\n", filename, buf.st_size);
    //printf("Pre malloc\n");
    src = malloc(buf.st_size);
    if (!src)
      return 1;
    /*fname is filename that needs to be "decrypted"
    in reality if you encrypt_file fname the same way it'll work 
    encrypt_file(fname, n) where n is encrypt_fileion mode
    if you choose incorrect n the file will not be found 
    */
    
    r = yffs_read(&fs, directory, filename, src, buf.st_size);
    if (r < 0)
      return 1;
    fwrite(src, 1, r, stdout);
    free(src);
  }
  yffs_umount(&fs);
	
  return 0;
}
