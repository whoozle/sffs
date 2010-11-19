#include "sffs.h"
#include <stdlib.h>
#include <endian.h>
#include <string.h>
#include <time.h>
/*SEEK_XXX macros. Consider to move 'em to sffs.h with similar name*/
#include <unistd.h>

#define SFFS_HEADER_SIZE (16)

static int sffs_entry_compare(const void *a, const void *b) {
	struct sffs_entry *ea = (struct sffs_entry *)a;
	struct sffs_entry *eb = (struct sffs_entry *)b;
	int d = strcmp(ea->name, eb->name);
	if (d)
		return d;
	return ea->block.mtime - eb->block.mtime;
}

static int sffs_block_compare(const void *a, const void *b) {
	struct sffs_block *ea = (struct sffs_block *)a;
	struct sffs_block *eb = (struct sffs_block *)b;
	return ea->begin - eb->begin;
}

static int sffs_vector_resize(struct sffs_vector *vec, size_t new_size) {
	uint8_t *p = (uint8_t *)realloc(vec->ptr, new_size);
	if (!p && new_size) {
		LOG_ERROR(("realloc(%p, %zu) failed", vec->ptr, new_size));
		return -1;
	}
	vec->ptr = p;
	vec->size = new_size;
	return 0;
}

static uint8_t *sffs_vector_insert(struct sffs_vector *vec, size_t pos, size_t size) {
	//LOG_DEBUG(("sffs_vector_insert(%zu, %zu)", pos, size));
	size_t new_size = vec->size + size;
	uint8_t *p = (uint8_t *)realloc(vec->ptr, new_size), *entry;
	if (!p) {
		return 0;
	}
	vec->ptr = p;
	pos *= size;
	entry = vec->ptr + pos;
	memmove(entry + size, entry, vec->size - pos);
	vec->size = new_size;
	return entry;
}

static int sffs_vector_remove(struct sffs_vector *vec, size_t pos, size_t size) {
	uint8_t *p;

	pos *= size;
	memmove(vec->ptr + pos, vec->ptr + pos + size, vec->size - pos - size);
	p = (uint8_t *)realloc(vec->ptr, vec->size - size);
	if (p)
		vec->ptr = p;
	vec->size -= size;
	return 0;
}

uint8_t *sffs_vector_append(struct sffs_vector *vec, size_t size) {
	size_t old_size = vec->size;
	if (sffs_vector_resize(vec, old_size + size) == -1)
		return 0;
	return vec->ptr + old_size;
}

const char* sffs_filename(struct sffs *fs, size_t index) {
	index *= sizeof(struct sffs_entry);
	if (index < fs->files.size) {
		return ((struct sffs_entry *)(fs->files.ptr + index))->name;
	} else
		return 0;
}

static int sffs_write_empty_header(struct sffs *fs, off_t offset, size_t size) {
	char header[SFFS_HEADER_SIZE] = {
		0, 0, 0, 0, 0, /*1:flags(empty) + size, current 1st*/
		0, 0, 0, 0, 0, /*2:flags + size*/
		0, 0, 0, 0, 0, 0, /*mtime + padding + fname len */
	};

	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		LOG_ERROR(("seek(%zu, SEEK_SET) failed", offset));
		return -1;
	}
	
	/* *((uint32_t *)&header[10]) = htole32(time(0)); //splitting block is not considered as modification to avoid fragmentation.*/
	*((uint32_t *)&header[1]) = htole32(size - sizeof(header));

	return (fs->write(&header, sizeof(header)) == sizeof(header))? 0: -1;
}

static int sffs_write_at(struct sffs *fs, off_t offset, const void *data, size_t size) {
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		LOG_ERROR(("seek(%zu, SEEK_SET) failed", offset));
		return -1;
	}

	if (fs->write(data, size) != size) {
		LOG_ERROR(("write(%p, %zu) failed", data, size));
		return -1;
	}

	return 0;
}

static int sffs_write_metadata(struct sffs *fs, off_t offset, uint8_t flags, uint32_t size, uint8_t fname_len, uint8_t padding) {
	uint8_t header[10], header_offset;
	uint8_t header2[4 + 1 + 1]; /*timestamp + padding + filename len*/
	
	if (size > fs->device_size) {
		LOG_ERROR(("cancelling sffs_write_metadata(0x%zx, %02x, %zu), do not corrupting filesystem!", offset, flags, size));
		return -1;
	}
	
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		LOG_ERROR(("seek(%zu, SEEK_SET) failed", offset));
		return -1;
	}

	if (fs->read(header, sizeof(header)) != sizeof(header)) {
		LOG_ERROR(("read(header) failed"));
		return -1;
	}
	
	header_offset = (header[0] & 0x80)? 5: 0;
	header_offset = 5 - header_offset;
	header[header_offset] &= 0x80;
	header[header_offset] |= flags;
	*((uint32_t *)&header[header_offset + 1]) = htole32(size);
	
	if (sffs_write_at(fs, offset + header_offset, header + header_offset, 5) == -1)
		return -1;
	
	/* update timestamp */
	*((uint32_t *)header2) = htole32(time(0));
	header2[4] = padding;
	header2[5] = fname_len;

	if (sffs_write_at(fs, offset + 10, header2, 6) == -1)
		return -1;

	return 0;
}

static int sffs_commit_metadata(struct sffs *fs, off_t offset) {
	uint8_t flag;
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		LOG_ERROR(("seek(%zu, SEEK_SET) failed", offset));
		return -1;
	}

	if (fs->read(&flag, 1) != 1) {
		LOG_ERROR(("reading flag failed"));
		return -1;
	}

	flag ^= 0x80;
	if (fs->seek(-1, SEEK_CUR) == (off_t)-1) {
		LOG_ERROR(("seek(-1, SEEK_CUR) failed"));
		return -1;
	}
	
	if (fs->write(&flag, 1) != 1) {
		LOG_ERROR(("write(commit flag) failed"));
		return -1;
	}

	return (flag & 0x80) != 0? 1: 0;
}

static long sffs_find_file(struct sffs *fs, const char *fname) {
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

static int sffs_compact(struct sffs *fs) {
	struct sffs_block *free = (struct sffs_block *)fs->free.ptr;
	size_t i, n = fs->free.size / sizeof(struct sffs_block);
	if (n < 2)
		return 0;
	LOG_DEBUG(("SFFS: compacting free space..."));
	--n;
	for(i = 0; i < n; ) {
		size_t j = i + 1;
		if (free[i].end == free[j].begin) {
			size_t size = free[j].end - free[i].begin;
			if (sffs_write_metadata(fs, free[i].begin, 0, size - SFFS_HEADER_SIZE, 0, 0) == -1)
				return -1;
			if (sffs_commit_metadata(fs, free[i].begin) == -1)
				return -1;
			free[i].end = free[j].end;
			if (sffs_vector_remove(&fs->free, j, sizeof(struct sffs_block)) == -1)
				return -1;
			--n;
			free = (struct sffs_block *)fs->free.ptr; /*might be relocated*/
		} else
			++i;
	}
	return 0;
}

static int sffs_recover_and_remove_old_files(struct sffs *fs) {
	struct sffs_entry *files = (struct sffs_entry *)fs->files.ptr;
	size_t i, n = fs->files.size / sizeof(struct sffs_entry);
	if (n < 2)
		return 0;
	--n;
	for(i = 0; i < n; ) {
		size_t j = i + 1;
		struct sffs_entry *file = files + i;
		if (strcmp(file->name, files[j].name) == 0) {
			size_t size = file->block.end - file->block.begin;
			LOG_INFO(("unlinking older file %s@%u vs %u", file->name, (unsigned)file->block.mtime, (unsigned)files[j].block.mtime));
			if (sffs_write_metadata(fs, file->block.begin, 0, size - SFFS_HEADER_SIZE, 0, 0) == -1)
				return -1;
			if (sffs_commit_metadata(fs, file->block.begin) == -1)
				return -1;
			free(file->name);
			if (sffs_vector_remove(&fs->files, i, sizeof(struct sffs_entry)) == -1)
				return -1;
			--n;
			files = (struct sffs_entry *)fs->files.ptr;
		} else
			++i;
	}
	return 0;
}

static int sffs_unlink_at(struct sffs *fs, size_t pos) {
	struct sffs_entry *file = ((struct sffs_entry *)fs->files.ptr) + pos;
	struct sffs_block *free_block;
	LOG_DEBUG(("erasing metadata[%zu]:%s at 0x%zx-0x%zx", pos, file->name, file->block.begin, file->block.end));
	if (sffs_write_metadata(fs, file->block.begin, 0, file->block.end - file->block.begin - SFFS_HEADER_SIZE, 0, 0) == -1)
		return -1;
	if (sffs_commit_metadata(fs, file->block.begin) == -1)
		return -1;
	
	free_block = (struct sffs_block *)sffs_vector_append(&fs->free, sizeof(struct sffs_block));
	*free_block = file->block;
	
	{
		size_t free_blocks = fs->free.size / sizeof(struct sffs_block);
		qsort(fs->free.ptr, free_blocks, sizeof(struct sffs_block), sffs_block_compare);
	}
	
	free(file->name);
	sffs_vector_remove(&fs->files, pos, sizeof(struct sffs_entry));
	return sffs_compact(fs);
}

int sffs_unlink(struct sffs *fs, const char *fname) {
	long pos = sffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	return sffs_unlink_at(fs, pos);
}

static struct sffs_block *find_best_free(struct sffs *fs, size_t full_size) {
	struct sffs_block *free_begin = (struct sffs_block *)fs->free.ptr;
	struct sffs_block *free_end = (struct sffs_block *)(fs->free.ptr + fs->free.size);
	struct sffs_block *free, *best_free = 0;
	size_t best_size = 0;
	uint32_t best_mtime = 0;
	for(free = free_begin; free < free_end; ++free) {
		size_t free_size = free->end - free->begin;
		if (free_size >= full_size) {
			if (!best_free || free_size < best_size || best_mtime > free->mtime) {
				best_free = free;
				best_size = free_size;
				best_mtime = free->mtime;
			}
		}
	}
	return best_free;
}

size_t sffs_get_largest_free(struct sffs *fs) {
	struct sffs_block *free_begin = (struct sffs_block *)fs->free.ptr;
	struct sffs_block *free_end = (struct sffs_block *)(fs->free.ptr + fs->free.size);
	struct sffs_block *free;
	size_t largest = 0;

	for(free = free_begin; free < free_end; ++free) {
		size_t size = free->end - free->begin;
		if (size > largest)
			largest = size;
	}
	return largest;
}

size_t sffs_get_total_free(struct sffs *fs) {
	struct sffs_block *free_begin = (struct sffs_block *)fs->free.ptr;
	struct sffs_block *free_end = (struct sffs_block *)(fs->free.ptr + fs->free.size);
	struct sffs_block *free;
	size_t total = 0;

	for(free = free_begin; free < free_end; ++free) {
		total += free->end - free->begin;
	}
	return total;
}

ssize_t sffs_write(struct sffs *fs, const char *fname, const void *data, size_t size) {
	size_t fname_len = strlen(fname), header_size, full_size, best_size, tail_size;
	struct sffs_entry *file;
	long remove_me;
	struct sffs_block *best_free;
	off_t offset;
	uint8_t padding;

	long pos = sffs_find_file(fs, fname);
	if (pos < 0) {
		remove_me = -1;
		pos = -pos - 1;
	} else {
		remove_me = pos + 1;
		LOG_DEBUG(("SFFS: old file in position %ld", remove_me));
	}
	file = (struct sffs_entry *)sffs_vector_insert(&fs->files, pos, sizeof(struct sffs_entry));
	if (!file) {
		LOG_ERROR(("SFFS: inserting file struct failed!"));
		return -1;
	}
	/*LOG_DEBUG(("file[pos] = %s, file[pos + 1] = %s", ((struct sffs_entry *)fs->files.ptr)[pos].name, ((struct sffs_entry *)fs->files.ptr)[pos + 1].name));*/

	file->name = strdup(fname);
	header_size = fname_len + SFFS_HEADER_SIZE; /* 2 * (flags + size) + mtime + padding + fname len + filename */
	full_size = size + header_size;
	best_free = find_best_free(fs, full_size);
	if (!best_free) {
		LOG_ERROR(("SFFS: no space left on device"));
		return -1;
	}

	best_size = best_free->end - best_free->begin;
	tail_size = best_size - full_size;
	offset = best_free->begin;
	if (tail_size > SFFS_HEADER_SIZE) {
		/*mark up empty block inside*/
		best_free->begin = offset + full_size;
		if (sffs_write_empty_header(fs, best_free->begin, tail_size) == -1) 
			return -1;
		padding = 0;
	} else {
		/* tail_size == SFFS_HEADER_SIZE or less */
		padding = tail_size;
		sffs_vector_remove(&fs->free, best_free - (struct sffs_block *)fs->free.ptr, sizeof(struct sffs_block));
	}
	LOG_DEBUG(("SFFS: creating file in position %ld -> 0x%lx", pos, (unsigned long)offset));
	file->block.begin = offset;
	file->block.end = offset + size + fname_len + padding;
	file->padding = padding;
	file->size = size;

	/*writing data*/
	if (sffs_write_at(fs, offset + header_size, data, size) == -1)
		return -1;
	/*writing metadata*/
	if (sffs_write_metadata(fs, offset, 0x40, size + fname_len + padding, fname_len, padding) == -1)
		return -1;
	/*write filename*/
	if (fs->write(fname, fname_len) != fname_len) {
		LOG_ERROR(("error writing filename (len: %zu)", fname_len));
		return -1;
	}
	/*commit*/
	if (sffs_commit_metadata(fs, offset) == -1)
		return -1;

	if (remove_me >= 0) {
		/*removing old copy*/
		sffs_unlink_at(fs, remove_me);
	}
	return size;
}

ssize_t sffs_read(struct sffs *fs, const char *fname, void *data, size_t size) {
	struct sffs_entry *file;
	long pos = sffs_find_file(fs, fname), offset;

	if (pos < 0)
		return -1;

	file = ((struct sffs_entry *)fs->files.ptr) + pos;
	if (size > file->size)
		size = file->size;
	
	offset = file->block.begin + SFFS_HEADER_SIZE + strlen(fname);
	LOG_DEBUG(("reading from offset: %lu, size: %zu", (unsigned long)offset, size));
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		LOG_ERROR(("seek(%ld, SEEK_SET) failed", offset));
		return -1;
	}
	return fs->read(data, size);
}

int sffs_stat(struct sffs *fs, const char *fname, struct stat *buf) {
	struct sffs_entry *file;
	long pos = sffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	memset(buf, 0, sizeof(*buf));
	file = ((struct sffs_entry *)fs->files.ptr) + pos;
	buf->st_size = file->size;
	buf->st_mtime = buf->st_ctime = file->block.mtime;
	return 0;
}

int sffs_format(struct sffs *fs) {
	return sffs_write_empty_header(fs, 0, fs->device_size);
}


int sffs_mount(struct sffs *fs) {
	off_t size = fs->seek(0, SEEK_END);

	if (size == (off_t)-1) {
		LOG_ERROR(("cannot obtain device size: seek(0, SEEK_END) failed"));
		return -1;
	}

	LOG_DEBUG(("SFFS: device size: %zu bytes", size));
	fs->device_size = size;
	
	size = fs->seek(0, SEEK_SET);
	if (size == (off_t)-1) {
		LOG_ERROR(("seek(0, SEEK_SET) failed"));
		return -1;
	}
	
	fs->files.ptr = 0;
	fs->files.size = 0;
	fs->free.ptr = 0;
	fs->free.size = 0;
	
	for(;;) {
		struct sffs_block block;
		
		unsigned header_off, committed, block_size;
		uint8_t header[SFFS_HEADER_SIZE];

		block.begin = fs->seek(0, SEEK_CUR); /* tell */
		if (block.begin >= fs->device_size)
			break;
		
		if (fs->read(header, sizeof(header)) != sizeof(header))
			break;
		
		header_off = (header[0] & 0x80)? 5: 0;
		committed = header[header_off] & 0x40;

		block_size = le32toh(*(uint32_t *)(header + header_off + 1));
		block.end = block.begin + sizeof(header) + block_size;
		block.mtime = (time_t)*(uint32_t *)&header[10];
		
		if (block.end == (off_t)-1) {
			LOG_ERROR(("SFFS: out of bounds"));
			return -1;
		}
		
		if (committed) {
			struct sffs_entry *file;
			size_t file_offset = fs->files.size;
			uint8_t filename_len = header[15], padding = header[14];

			if (filename_len == 0 || filename_len == 0xff)
				break;
		
			if (sffs_vector_resize(&fs->files, file_offset + sizeof(struct sffs_entry)) == -1)
				return -1;
			
			file = (struct sffs_entry *)((char *)fs->files.ptr + file_offset);
			file->name = malloc(filename_len + 1);
			if (!file->name) {
				LOG_ERROR(("filename allocation failed(%u)", (unsigned)filename_len));
				return -1;
			}
			if (fs->read(file->name, filename_len) != filename_len) {
				LOG_ERROR(("read(%u) failed", (unsigned)filename_len));
				return -1;
			}
			file->size = block_size - filename_len - padding;
			file->name[filename_len] = 0;
			LOG_DEBUG(("SFFS: read file %s -> %zu", file->name, file->size));
			file->block = block;
		} else {
			struct sffs_block *free;
			size_t free_offset = fs->free.size;
			if (sffs_vector_resize(&fs->free, free_offset + sizeof(struct sffs_block)) == -1)
				return -1;
			free = (struct sffs_block *)((char *)fs->free.ptr + free_offset);
			*free = block;
			LOG_DEBUG(("SFFS: free space %zu->%zu", block.begin, block.end));
			if (free->end > fs->device_size)  {
				LOG_ERROR(("SFFS: free spaces crosses device bound!"));
				free->end = fs->device_size;
				break;
			}
		}
		if (fs->seek(block.end, SEEK_SET) == (off_t)-1) {
			LOG_ERROR(("SFFS: block end %zu is out of bounds", block.end));
			break;
		}
	}
	if (fs->files.size == 0 && fs->free.size == 0) {
		LOG_ERROR(("SFFS: corrupted file system: no free blocks or files."));
		return -1;
	}
	
	{
		size_t files = fs->files.size / sizeof(struct sffs_entry);
		qsort(fs->files.ptr, files, sizeof(struct sffs_entry), sffs_entry_compare);
	}
	if (sffs_recover_and_remove_old_files(fs) == -1)
		return -1;
	if (sffs_compact(fs) == -1)
		return -1;
	LOG_DEBUG(("SFFS: mounted!"));
	return 0;
}



int sffs_umount(struct sffs *fs) {
	free(fs->files.ptr);
	fs->files.ptr = 0;
	free(fs->free.ptr);
	fs->free.ptr = 0;
	return 0;
}
