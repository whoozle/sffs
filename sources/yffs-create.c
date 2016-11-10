/*Team 22*/

#include "yffs.h"
#define yffs_HEADER_SIZE (16)

/*
* This is used to initalize a newly created yffs fs.  
*
* Args: Struct 'yffs' - FS Container
*		Struct 'yffs_block' - Pointer to empty first block
*
* Out:  Int - Indicates success of failure of writing the empty header to the first block.
*/

static int yffs_write_empty_header(struct yffs *fs, struct yffs_block *block) {
    size_t size = block->end - block->begin;
    if (size <= yffs_HEADER_SIZE) {
        LOG_ERROR(("yffs: avoid writing block <= 16b."));
        return -1;
    }
    char header[yffs_HEADER_SIZE] = { //'header' indexes used: 0, 1, 10, 12, 13, 14, 15
        0, 0, 0, 0, 0, /*1:flags(empty) + size, current 1st*/
        0, 0, 0, 0, 0, /*2:flags + size*/
        0, 0, 0, 0, 0, 0, /*mtime + padding + fname len */
    };
    
    if (fs->seek(block->begin, SEEK_SET) == (off_t)-1) {
        LOG_ERROR(("yffs: seek(%zu, SEEK_SET) failed", block->begin));
        return -1;
    }
    
    *((uint32_t *)&header[1]) = htole32(size - yffs_HEADER_SIZE);
    
    return (fs->write(&header, sizeof(header)) == sizeof(header))? 0: -1; 
}

/*
* This is used to initalize a newly created yffs fs.  
*
* Args: Struct 'yffs' - FS Container
* Out:  Int - Indicates success or failure of adding a initial block to YFFS.
*/
int yffs_format(struct yffs *fs) {
    struct yffs_block first_block;
    first_block.begin = 0;
    first_block.end = fs->device_size;
    return yffs_write_empty_header(fs, &first_block);
}

static int fd;
static ssize_t fs_write_func(const void *ptr, size_t size) { return write(fd, ptr, size); }
static ssize_t fs_read_func(void *ptr, size_t size) { return read(fd, ptr, size); }
static off_t fs_seek_func(off_t offset, int whence) { return lseek(fd, offset, whence); }

/* YFFS
	This function is used to create a new YFFS filesystem. 
	
	Args:  FS Name = argv[1] FS Size = argv[2].
	Out: Yffs
	
	Ex. yffs-create fsname 10000
*/

int main(int argc, char **argv) {
	struct yffs fs;
	if (argc < 3) {
		printf("Usage: createfs: ./yffs-create fsname 10000\n");
		return 0;
	}

	fs.write = fs_write_func; //!! when yffs calls 'write' it is actually calling fs_write which is defined above.
	fs.read = fs_read_func;
	fs.seek = fs_seek_func;
	fs.device_size = atoi(argv[2]);
	
	if (fs.device_size < 32) {
		printf("YFFS must be greater then 32 bytes!\n"); //This is to allow the inital empty header and one additional memory allocation.
		return 1;
	}
	
	{
		fd = open(argv[1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR ); //Create a file'system' at this location, w/ permissions.
		if (fd == -1) {
			perror("open");
			return 1;
		}
		yffs_format(&fs); //This takes the new fs, and adds a 'first block' with an empty header.
		close(fd);
		
		if (truncate(argv[1], fs.device_size) == -1)
			perror("truncate");
	}
	//Only print output on errors
	//printf("YFFS creation successful!\n \'%s\' -> size: %u bytes\n",argv[1], (unsigned)fs.device_size);
	return 0;
}

