#include "sffs.h"
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <string.h>
#include <time.h>

static int sffs_entry_compare(const void *a, const void *b) {
	struct sffs_entry *ea = (struct sffs_entry *)a;
	struct sffs_entry *eb = (struct sffs_entry *)b;
	return strcmp(ea->name, eb->name);
}

static int sffs_vector_resize(struct sffs_vector *vec, size_t new_size) {
	uint8_t *p = (uint8_t *)realloc(vec->ptr, new_size);
	if (!p && new_size) {
		perror("realloc");
		return -1;
	}
	vec->ptr = p;
	vec->size = new_size;
	return 0;
}

static uint8_t *sffs_vector_insert(struct sffs_vector *vec, size_t pos, size_t size) {
	uint8_t *p = (uint8_t *)realloc(vec->ptr, vec->size + size), *entry;
	if (!p) {
		perror("realloc");
		return 0;
	}
	vec->ptr = p;
	pos *= size;
	entry = vec->ptr + pos;
	memmove(entry + size, entry, vec->size - pos);
	memset(entry, 0, size);
	vec->size += size;
	return entry;
}

static int sffs_vector_remove(struct sffs_vector *vec, size_t pos, size_t size) {
	return 0;
}

uint8_t *sffs_vector_append(struct sffs_vector *vec, size_t size) {
	size_t old_size = vec->size;
	if (sffs_vector_resize(vec, old_size + size) == -1)
		return 0;
	return vec->ptr + old_size;
}

static int sffs_write_empty_header(struct sffs *fs, off_t offset, size_t size) {
	printf("writing empty header at %zx (block size: %zx)\n", offset, size);
	char header[16] = {
		0, 0, 0, 0, 0, /*1:flags(empty) + size, current 1st*/
		0, 0, 0, 0, 0, /*2:flags + size*/
		0, 0, 0, 0, 0, 0, /*mtime + padding + fname len */
	};

	if (fs->seek(offset, SEEK_SET) == (off_t)-1)
		return -1;
	
	//*((uint32_t *)&header[10]) = htole32(time(0)); //splitting block is not considered as modification to avoid fragmentation.
	*((uint32_t *)&header[1]) = htole32(size - sizeof(header));

	return (fs->write(&header, sizeof(header)) == sizeof(header))? 0: -1;
}

static int sffs_write_at(struct sffs *fs, off_t offset, const void *data, size_t size) {
	if (fs->seek(offset, SEEK_SET) == (off_t)-1)
		return -1;

	if (fs->write(data, size) != size)
		return -1;

	return 0;
}

static int sffs_write_metadata(struct sffs *fs, off_t offset, uint8_t flags, uint32_t size, uint8_t fname_len, uint8_t padding) {
	if (fs->seek(offset, SEEK_SET) == (off_t)-1)
		return -1;

	uint8_t header[10];
	if (fs->read(header, sizeof(header)) != sizeof(header))
		return -1;
	
	int header_offset = (header[0] & 0x80)? 5: 0;
	header_offset = 5 - header_offset;
	header[header_offset] = flags;
	*((uint32_t *)&header[header_offset + 1]) = htole32(size); //internal block size
	
	if (sffs_write_at(fs, offset + header_offset, header + header_offset, 5) == -1)
		return -1;
	
	/* update timestamp */
	uint8_t header2[4 + 1 + 1]; /*timestamp + padding + filename len*/
	*((uint32_t *)header2) = htole32(time(0));
	header2[4] = padding;
	header2[5] = fname_len;

	if (sffs_write_at(fs, offset + 10, header2, 6) == -1)
		return -1;

	return 0;
}

static int sffs_commit_metadata(struct sffs *fs, off_t offset) {
	uint8_t flag;
	if (fs->seek(offset, SEEK_SET) == (off_t)-1)
		return -1;

	if (fs->read(&flag, 1) != 1)
		return -1;

	flag ^= 0x80;
	if (fs->seek(-1, SEEK_CUR) == (off_t)-1)
		return -1;
	
	if (fs->write(&flag, 1) != 1)
		return -1;

	return (flag & 0x80) != 0? 1: 0;
}

static int sffs_find_file(struct sffs *fs, const char *fname) {
	struct sffs_entry *files = (struct sffs_entry *)fs->files.ptr;
	int first = 0, last = fs->files.size / sizeof(struct sffs_entry);
	while(first < last) {
		int mid = (first + last) / 2;
		int cmp = strcmp(fname, files[mid].name);
		if (cmp > 0) {
			first = mid + 1;
		} else if (cmp < 0) {
			last = mid;
		} else
			return mid;
	}
	return -(first + 1);
}

ssize_t sffs_write(struct sffs *fs, const char *fname, const void *data, size_t size) {
	size_t fname_len = strlen(fname);
	int pos = sffs_find_file(fs, fname);
	if (pos < 0) {
		struct sffs_entry *file;
		pos = -pos - 1;
		printf("inserting at position %d\n", pos);
		file = (struct sffs_entry *)sffs_vector_insert(&fs->files, pos, sizeof(struct sffs_entry));
		if (!file) {
			printf("failed!\n");
			return -1;
		}
		file->name = strdup(fname);
		size_t header_size = fname_len + 16; /* 2 * (flags + size) + mtime + padding + fname len + filename */
		size_t full_size = size + header_size;
		struct sffs_block *free_begin = (struct sffs_block *)fs->free.ptr;
		struct sffs_block *free_end = (struct sffs_block *)(fs->free.ptr + fs->free.size);
		struct sffs_block *best_free = 0;
		size_t best_size = 0;
		for(struct sffs_block *free = free_begin; free < free_end; ++free) {
			size_t free_size = free->end - free->begin;
			if (free_size >= full_size) {
				if (!best_free || free_size < best_size) {
					best_free = free;
					best_size = free_size;
				}
			}
		}
		if (!best_free)
			return -1;

		printf("SFFS: found free block (max size: %zu)\n", best_size);
		size_t tail_size = best_size - full_size;
		if (tail_size > 255) {
			off_t offset = best_free->begin;
			printf("SFFS: too large, splitting, writing new free-block header\n");
			/*mark up empty block inside*/
			best_free->begin = offset + full_size;
			if (sffs_write_empty_header(fs, best_free->begin, tail_size) == -1)
				return -1;
			//return 0;
			/*writing data*/
			if (sffs_write_at(fs, offset + header_size, data, size) == -1)
				return -1;
			/*writing metadata*/
			if (sffs_write_metadata(fs, offset, 0x40, size + fname_len, fname_len, 0) == -1)
				return -1;
			/*write filename*/
			if (fs->write(fname, fname_len) != fname_len)
				return -1;
			/*commit*/
			if (sffs_commit_metadata(fs, offset) == -1)
				return -1;
		}
	} else {
		fprintf(stderr, "not implemented\n");
	}
	return size;
}

ssize_t sffs_read(struct sffs *fs, const char *fname, void *data, size_t size) {
	int pos = sffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	struct sffs_entry *file = ((struct sffs_entry *)fs->files.ptr) + pos;
	if (size > file->size)
		size = file->size;
	
	if (fs->seek(file->block.begin + 16 + strlen(fname), SEEK_SET) == (off_t)-1)
		return -1;
	return fs->read(data, size);
}

int sffs_unlink(struct sffs *fs, const char *fname) {
	int pos = sffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	struct sffs_entry *file = ((struct sffs_entry *)fs->files.ptr) + pos;
	sffs_write_metadata(fs, file->block.begin, 0, file->block.end - file->block.begin - 16, 0, 0);
	sffs_commit_metadata(fs, file->block.begin);
	
	struct sffs_block *free = (struct sffs_block *)sffs_vector_append(&fs->free, sizeof(struct sffs_block));
	*free = file->block;
	
	sffs_vector_remove(&fs->files, pos, sizeof(struct sffs_entry));
	return 0;
}

int sffs_stat(struct sffs *fs, const char *fname, struct stat *buf) {
	int pos = sffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	memset(buf, 0, sizeof(*buf));
	struct sffs_entry *file = ((struct sffs_entry *)fs->files.ptr) + pos;
	buf->st_size = file->size;
	buf->st_mtime = buf->st_ctime = file->block.mtime;
	return 0;
}

int sffs_format(struct sffs *fs) {
	return sffs_write_empty_header(fs, 0, fs->device_size);
}

int sffs_mount(struct sffs *fs) {
	off_t size = fs->seek(0, SEEK_END);

	if (size == (off_t)-1)
		return -1;

	fprintf(stderr, "SFFS: device size: %zu bytes\n", size);
	fs->device_size = size;
	
	/*reading journal*/
	size = fs->seek(0, SEEK_SET);
	if (size == (off_t)-1)
		return -1;
	
	fprintf(stderr, "SFFS: reading metadata\n");
	fs->files.ptr = 0;
	fs->files.size = 0;
	fs->free.ptr = 0;
	fs->free.size = 0;
	
	for(;;) {
		struct sffs_block block;
		
		unsigned header_off, committed, block_size;
		uint8_t header[16];

		block.begin = fs->seek(0, SEEK_CUR); /* tell */
		
		if (fs->read(header, sizeof(header)) != sizeof(header))
			break;
		
		header_off = (header[0] & 0x80)? 5: 0;
		committed = header[header_off] & 0x40;

		block_size = le32toh(*(uint32_t *)(header + header_off + 1));
		block.end = block.begin + sizeof(header) + block_size;
		block.mtime = (time_t)*(uint32_t *)&header[10];
		
		if (block.end == (off_t)-1) {
			fprintf(stderr, "SFFS: out of bounds\n");
			return 1;
		}
		
		if (committed) {
			uint8_t filename_len = header[15], padding = header[14];
			if (filename_len == 0 || filename_len == 0xff)
				break;
		
			struct sffs_entry *file;
			size_t file_offset = fs->files.size;
			if (sffs_vector_resize(&fs->files, file_offset + sizeof(struct sffs_entry)) == -1)
				return 1;
			
			file = (struct sffs_entry *)((char *)fs->files.ptr + file_offset);
			file->name = malloc(filename_len + 1);
			if (!file->name) {
				perror("malloc");
				return 1;
			}
			if (fs->read(file->name, filename_len) != filename_len) {
				return 1;
			}
			file->size = block_size - filename_len - padding;
			file->name[filename_len] = 0;
			printf("read file %s -> %zu\n", file->name, file->size);
			file->block = block;
		} else {
			struct sffs_block *free;
			size_t free_offset = fs->free.size;
			if (sffs_vector_resize(&fs->free, free_offset + sizeof(struct sffs_block)) == -1)
				return 1;
			free = (struct sffs_block *)((char *)fs->free.ptr + free_offset);
			*free = block;
			printf("free space %zx->%zx\n", block.begin, block.end);
			if (free->end > fs->device_size)  {
				printf("SFFS: free spaces crosses device bound!\n");
				free->end = fs->device_size;
				break;
			}
		}
		if (fs->seek(block.end, SEEK_SET) == (off_t)-1) {
			fprintf(stderr, "%zu is out of bounds\n", block.end);
			break;
		}
	}
	
	{
		size_t files = fs->files.size / sizeof(struct sffs_entry);
		fprintf(stderr, "SFFS: sorting %zu files...\n", files);
		qsort(fs->files.ptr, files, sizeof(struct sffs_entry), sffs_entry_compare);
	}

	fprintf(stderr, "SFFS: mounted!\n");
	return 0;
}

int sffs_umount(struct sffs *fs) {
	return 0;
}
