#include "sffs.h"
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <string.h>

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
	int pos = sffs_find_file(fs, fname);
	if (pos < 0) {
		struct sffs_entry *file;
		pos = -pos - 1;
		printf("inserting at position %d\n", pos);
		file = (struct sffs_entry *)sffs_vector_insert(&fs->files, pos * sizeof(struct sffs_entry), sizeof(struct sffs_entry));
		if (!file) {
			printf("failed!\n");
			return -1;
		}
		file->name = strdup(fname);
	} else {
		fprintf(stderr, "not implemented\n");
	}
	return size;
}

ssize_t sffs_read(struct sffs *fs, const char *fname, void *data, size_t size) {
	
	return size;
}

int sffs_format(struct sffs *fs) {
	char z = 0;
	if (fs->seek(0, SEEK_SET) == (off_t)-1)
		return -1;
	if (fs->write(&z, 1) != 1)
		return -1;
	return 0;
}

int sffs_mount(struct sffs *fs) {
	off_t size = fs->seek(0, SEEK_END);
	if (size == (off_t)-1)
		return -1;
	fprintf(stderr, "SFFS: device size: %u bytes\n", (unsigned)size);
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
		unsigned header_off, committed, file_size;
		off_t data_begin, data_end;
		uint8_t header[11], filename_len;
		
		if (fs->read(header, 11) != 11) 
			break;
		
		filename_len = header[10];
		if (filename_len == 0 || filename_len == 0xff)
			break;
		
		header_off = (header[0] & 0x80)? 5: 0;
		committed = header[header_off] & 0x40;
		file_size = le32toh(*(uint32_t *)(header + header_off + 1));
		data_begin = fs->seek(0, SEEK_CUR); /* tell */
		data_end = fs->seek(file_size, SEEK_CUR);
		if (data_end == (off_t)-1) {
			fprintf(stderr, "SFFS: out of bounds\n");
			return 1;
		}
		if (committed) {
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
			file->name[filename_len] = 0;
			file->area.begin = data_begin;
			file->area.end = data_end;
		} else {
			struct sffs_area *free;
			size_t free_offset = fs->free.size;
			if (sffs_vector_resize(&fs->free, free_offset + sizeof(struct sffs_area)) == -1)
				return 1;
			free = (struct sffs_area *)((char *)fs->free.ptr + free_offset);
			free->begin = data_begin;
			free->end = data_end;
		}
	}
	
	{
		size_t files = fs->files.size / sizeof(struct sffs_entry);
		fprintf(stderr, "SFFS: sorting %u files...\n", (unsigned)files);
		qsort(fs->files.ptr, files, sizeof(struct sffs_entry), sffs_entry_compare);
	}

	fprintf(stderr, "SFFS: mounted!\n");
	return 0;
}

int sffs_umount(struct sffs *fs) {
	return 0;
}
