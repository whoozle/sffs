/*Team 22*/

#include "yffs.h"
#include <pthread.h>

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
	//opens argv[1] which is the fs
	fd = open(fname, O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	//mounts fs, returns an int error/success code
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
	/*
	TO DO:
		Redo arguments to accept a flag (-a, -r) 
		Usage: ./yffs-write filesystem original new
		Rewrite code to append or rewrite file
	*/
	struct yffs fs;
	pthread_mutex_init(&mutex, NULL);

	if (argc < 3) {
		printf("Usage:\n\n"
			   "\twrite:    ./yffs-write fsname.img newFile\n"
			"\twrite: 	./yffs-write fsname.img orignal new\n");	
		return 0;
	}

	fs.write = fs_write_func;
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;


//WRITE TO FILESYSTEM
		int f = 2;
		
		//mount system named at argv[1], return if it failed
		if (mount_image(&fs, argv[1]) == -1)
			return 2;

		FILE *tmp, *add;
		char byte;
		tmp = fopen("temp.txt", "ab");

		if (argc != 3) {
			f = 3; 
			if (access(argv[f], F_OK) == -1) {
				printf("File not found\n");
				exit(1);
			}
			struct stat buf;
			const char *fname = argv[2];
			void *src;
			ssize_t r;
			yffs_stat(&fs, fname, &buf);
			//printf("%s = %zu\n", fname, buf.st_size);
			if (buf.st_size > 10000) {
				printf("File not found\n");
				exit(1);
			}
			src = malloc(buf.st_size);
			r = yffs_read(&fs, fname, src, buf.st_size);
			//**decrypt somewhere around here**
			fwrite(src, 1, r, tmp);
			free(src);
		}
		
		if (argc > 4) {
			int i; 
			for (i = 3; i < argc; i++) {
			}

		} else {
			if (access(argv[f], F_OK) == -1) {
				printf("File not found\n");
				exit(1);
			}
			add = fopen(argv[f], "rb");
			while (!feof(add)) {
				fread(&byte, sizeof(char), 1, add);
				fwrite(&byte, sizeof(char), 1, tmp);
			}
		}
		fclose(tmp);
		fclose(add);
	
		//**Encrypt Here **
		
		//src_fd is the file descriptor for argv[f]
		int src_fd = -1;
			
		//offset
		off_t src_size = 0;
			
		//buffer
		void *src_data = 0;
			
		//printf("reading source %s...\n", argv[f]);
			
		//set src_fd to arv[f] 
	//	if ((src_fd = open(argv[f], O_RDONLY)) == -1) {
		if ((src_fd = open("temp.txt", O_RDONLY)) == -1) {
			perror("open");
		}
			
		//set size
		src_size = lseek(src_fd, 0, SEEK_END);
			
		//check if size is legitimate
		if (src_size == (off_t) -1) {
			perror("lseek");
			goto next;
		}
			
		//make space in memory for data
		src_data = malloc(src_size);
			
		//checks malloc
		if (src_data == NULL) {
			perror("malloc");
			goto next;
		}
	
		lseek(src_fd, 0, SEEK_SET);
			
		//reads from argv[f]->src_data
		if (read(src_fd, src_data, src_size) != src_size) {
			perror("short read");
			goto next;
		}
			
		//no longer need arv[f]
		close(src_fd);
		//printf("writing file %s\n", argv[f]);
			
		//writes the file
		//argv[2] is filename to obfuscate
        if (yffs_write(&fs, "abcdef", src_data, src_size) == -1)
		//if (yffs_write(&fs, argv[2], src_data, src_size) == -1)
			goto next;
#if 0
			memset(src_data, '@', src_size);
			yffs_read(&fs, argv[f], src_data, src_size);
			fwrite(src_data, 1, src_size, stdout);
#endif
		/* 'next' appears to just close everything
			implying it is only called IF there is nothing to write, i.e. no more args
		*/
		next:
			free(src_data);
			close(src_fd);
		
		//printf("unmounting...\n");
		yffs_umount(&fs);
		remove("temp.txt");
		
		close(fd);
		return 0;
	} 
	
