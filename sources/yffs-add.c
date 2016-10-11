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
static uint8_t emu_device[EMU_DEVICE_SIZE];
static unsigned emu_device_stat[EMU_DEVICE_SIZE];
static unsigned emu_device_pos;
static ssize_t emu_write_func(const void *ptr, size_t size); 
static ssize_t emu_read_func(void *ptr, size_t size); 
static off_t emu_seek_func(off_t offset, int whence); 


static ssize_t fs_write_func(const void *ptr, size_t size) {
  return write(fd, ptr, size);
}

static ssize_t fs_read_func(void *ptr, size_t size) {
  return read(fd, ptr, size);
}

static off_t fs_seek_func(off_t offset, int whence) {
  return lseek(fd, offset, whence);
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
  if (argc < 3) {
    printf("Usage: yffs-add fs.img filename\n");
    return 0;
  }

  fs.write = fs_write_func;
  fs.read = fs_read_func;
  fs.seek = fs_seek_func;

  //WRITE TO FILESYSTEM
  int f;
		
  if (mount_image(&fs, argv[1]) == -1)
    return 2;

  int src_fd = -1;
  off_t src_size = 0;
  void *src_data = 0;
			
  printf("reading source %s...\n", argv[2]);
  if ((src_fd = open(argv[2], O_RDONLY)) == -1) {
    perror("open");
    //continue;
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
  printf("writing file %s\n", argv[f]);
			
  if (yffs_write(&fs, argv[f], src_data, src_size) == -1)
    goto next;
#if 0
  memset(src_data, '@', src_size);
  yffs_read(&fs, argv[f], src_data, src_size);
  fwrite(src_data, 1, src_size, stdout);
#endif
 next:
  free(src_data);
  close(src_fd);
		
  printf("unmounting...\n");
  yffs_umount(&fs);
		
  close(fd);
	
  return 0;
}




//EMU
static ssize_t emu_write_func(const void *ptr, size_t size) {
  assert(emu_device_pos + size <= EMU_DEVICE_SIZE);
  for(size_t i = 0; i < size; ++i)
    ++emu_device_stat[emu_device_pos + i];
  memcpy(emu_device + emu_device_pos, ptr, size);
  emu_device_pos += size;
  return size;
}

static ssize_t emu_read_func(void *ptr, size_t size) {
  assert(emu_device_pos + size <= EMU_DEVICE_SIZE);
  memcpy(ptr, emu_device + emu_device_pos, size);
  emu_device_pos += size;
  return size;
}

static off_t emu_seek_func(off_t offset, int whence) {
  switch(whence) {
  case SEEK_SET:
    emu_device_pos = offset;
    break;
  case SEEK_CUR:
    emu_device_pos += offset;
    break;
  case SEEK_END:
    emu_device_pos = EMU_DEVICE_SIZE + offset;
    break;
  default:
    assert(0);
  }
  assert(emu_device_pos <= EMU_DEVICE_SIZE);
  return emu_device_pos;
}
