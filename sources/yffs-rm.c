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
	createfs: ./yffs-tool fsname.img createfs 10000
	write:    ./yffs-tool fsname.img write test.txt
    read:     ./yffs-tool fsname.img read test.txt
	remove:	  ./yffs-tool fsname.img remove test.txt
	list:     ./yffs-tool fsname.img list  
	test:	  ./yffs-tool fsname.img test
	wear:	  ./yffs-tool fsname.img wear
*/
int main(int argc, char **argv) {
	struct yffs fs;

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;


//REMOVE ITEM FROM FILESYSTEM	
	int f, recursive = 0;
    int mode = 0;
  if (argv[argc - 1][0] == '-') {
        mode = argv[argc - 1][1] - 48;
  }
	if (argc < 3) {
		printf("usage: remove:  ./yffs-tool fsname.img test.txt  \n");
		return 0;
	}

	for(int i = 0; i < argc; i++)
	{
		if(argv[i] == "-r")
			recursive = 1;
	}

	if (mount_image(&fs, argv[1]) == -1) {
		return 2;
	}

	for(f = 2; f < 3; ++f) {
		/* argv[f] is filename to encrypt
		encrypt(argv[f], n) where n is encryption method
		*/
		if (strcmp(argv[f], "-r") != 0){
		    int index = strlstchar(argv[f], '/');
		    char * directory = (char*)substring(argv[f], 0, index+1);
		    if(index == -1){
		      char * buff = "/";
		      directory = buff;
		    }

		    char * filename;
		    filename = (char *)substring(argv[f], index+1, strlen(argv[f]) - (index+1));
		    
			printf("Removed \'%s\'\n", filename);
			encrypt_file(filename, mode);
			yffs_unlink(&fs, filename, recursive);
		}
	}

	
	yffs_umount(&fs);
} 
