#include "yffs.h"
#define yffs_HEADER_SIZE (16)

//If you set this to 1 it will remove encryption/decryption, if that is causing other members problems
static int testing = 1;

int yffs_entry_compare(const void *a, const void *b) {
	struct yffs_entry *ea = (struct yffs_entry *)a;
	struct yffs_entry *eb = (struct yffs_entry *)b;
	int d = strcmp(ea->name, eb->name);
	if (d)
		return d;
	return ea->block.mtime - eb->block.mtime;
}

static int yffs_block_compare(const void *a, const void *b) {
	struct yffs_block *ea = (struct yffs_block *)a;
	struct yffs_block *eb = (struct yffs_block *)b;
	return ea->begin - eb->begin;
}

static int yffs_vector_resize(struct yffs_vector *vec, size_t new_size) {
	uint8_t *p = (uint8_t *)realloc(vec->ptr, new_size);
	if (!p && new_size) {
		//LOG_ERROR(("yffs: realloc(%p, %zu) failed", vec->ptr, new_size));
		return -1;
	}
	vec->ptr = p;
	vec->size = new_size;
	return 0;
}

static uint8_t *yffs_vector_insert(struct yffs_vector *vec, size_t pos, size_t size) {
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

static int yffs_vector_remove(struct yffs_vector *vec, size_t pos, size_t size) {
	uint8_t *p;

	pos *= size;
	memmove(vec->ptr + pos, vec->ptr + pos + size, vec->size - pos - size);
	p = (uint8_t *)realloc(vec->ptr, vec->size - size);
	if (p)
		vec->ptr = p;
	vec->size -= size;
	return 0;
}

uint8_t *yffs_vector_append(struct yffs_vector *vec, size_t size) {
	size_t old_size = vec->size;
	if (yffs_vector_resize(vec, old_size + size) == -1)
		return 0;
	return vec->ptr + old_size;
}

//gets the filename at 'index'
const char* yffs_filename(struct yffs *fs, size_t index, char * directory) {
	index *= sizeof(struct yffs_entry);
	if (index < fs->files.size) {
		if(strcmp(((struct yffs_entry *)(fs->files.ptr + index))->dir, directory) == 0 ){
			printf("Name is %s\n", ((struct yffs_entry *)(fs->files.ptr + index))->name);
			return ((struct yffs_entry *)(fs->files.ptr + index))->name;
		}
		else{
			printf("Name is %s\n", ((struct yffs_entry *)(fs->files.ptr + index))->dir);
			return ((struct yffs_entry *)(fs->files.ptr + index))->dir;
		}
	} else
		return 0;
}

//gets the permission bits at 'index'
const unsigned int yffs_permission(struct yffs *fs, size_t index) {
	index *= sizeof(struct yffs_entry);
	if (index < fs->files.size) {
		return ((struct yffs_entry *)(fs->files.ptr + index))->permBits;
	} else
		return 0;
}

static int yffs_write_empty_header(struct yffs *fs, struct yffs_block *block) {
	size_t size = block->end - block->begin;
	if (size <= yffs_HEADER_SIZE) {
		//LOG_ERROR(("yffs: avoid writing block <= 16b."));
		return -1;
	}
	char header[yffs_HEADER_SIZE] = {
		0, 0, 0, 0, 0, /*1:flags(empty) + size, current 1st*/
		0, 0, 0, 0, 0, /*2:flags + size*/
		0, 0, 0, 0, 0, 0, /*mtime + padding + fname len */
	};

	if (fs->seek(block->begin, SEEK_SET) == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(%zu, SEEK_SET) failed", block->begin));
		return -1;
	}
	
	*((uint32_t *)&header[1]) = htole32(size - yffs_HEADER_SIZE);

	return (fs->write(&header, sizeof(header)) == sizeof(header))? 0: -1;
}

static int yffs_write_at(struct yffs *fs, off_t offset, const void *data, size_t size) {
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(%zu, SEEK_SET) failed", offset));
		return -1;
	}

	if (fs->write(data, size) != size) {
		//LOG_ERROR(("yffs: write(%p, %zu) failed", data, size));
		return -1;
	}

	return 0;
}

static int yffs_write_metadata(struct yffs *fs, struct yffs_block *block, uint8_t flags, uint8_t fname_len, uint8_t padding) {
	uint8_t header[10], header_offset;
	uint8_t header2[4 + 1 + 1]; /*timestamp + padding + filename len*/
	
	uint32_t size = (uint32_t)(block->end - block->begin - yffs_HEADER_SIZE);
	if (block->begin + size > fs->device_size) {
		//LOG_ERROR(("yffs: cancelling yffs_write_metadata(0x%zx, %02x, %u), do not corrupt filesystem!", block->begin, flags, size));
		return -1;
	}
	
	if (fs->seek(block->begin, SEEK_SET) == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(%zu, SEEK_SET) failed", block->begin));
		return -1;
	}

	if (fs->read(header, sizeof(header)) != sizeof(header)) {
		//LOG_ERROR(("yffs: read(header) failed"));
		return -1;
	}
	
	header_offset = (header[0] & 0x80)? 5: 0;
	header_offset = 5 - header_offset;
	header[header_offset] &= 0x80;
	header[header_offset] |= flags;
	*((uint32_t *)&header[header_offset + 1]) = htole32(size);
	
	if (yffs_write_at(fs, block->begin + header_offset, header + header_offset, 5) == -1)
		return -1;
	
	/* update timestamp */
	*((uint32_t *)header2) = htole32(block->mtime);
	header2[4] = padding;
	header2[5] = fname_len;

	if (yffs_write_at(fs, block->begin + 10, header2, 6) == -1)
		return -1;

	return 0;
}

static int yffs_commit_metadata(struct yffs *fs, struct yffs_block *block) {
	uint8_t flag;
	if (fs->seek(block->begin, SEEK_SET) == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(%zu, SEEK_SET) failed", block->begin));
		return -1;
	}

	if (fs->read(&flag, 1) != 1) {
		//LOG_ERROR(("yffs: reading flag failed"));
		return -1;
	}

	flag ^= 0x80;
	if (fs->seek(-1, SEEK_CUR) == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(-1, SEEK_CUR) failed"));
		return -1;
	}
	
	if (fs->write(&flag, 1) != 1) {
		//LOG_ERROR(("yffs: write(commit flag) failed"));
		return -1;
	}

	return (flag & 0x80) != 0? 1: 0;
}

/*
returns a long which contains the number of yffs_entry structs into files.ptr that the 
file's entry is from the beginning of files.ptr
         0                  1                   2                  3   
|<---yffs_entry--->|<---yffs_entry--->|<---needThisOne--->|<---yffs_entry--->|

yffs_find_file would return 2, because it is entry number 2.

you would get a specific yffs_entry pointer from a filename like so (from yffs_read):
struct (yffs_entry *) my_file = ((struct yffs_entry *)fs->files.ptr) + yffs_find_file(fs, fname);
 */
static long yffs_find_file(struct yffs *fs, const char *fname) {
	struct yffs_entry *files = (struct yffs_entry *)fs->files.ptr;
	int first = 0, last = fs->files.size / sizeof(struct yffs_entry);
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

static int yffs_compact(struct yffs *fs) {
	struct yffs_block *free = (struct yffs_block *)fs->free.ptr;
	size_t i, n = fs->free.size / sizeof(struct yffs_block);
	if (n < 2)
		return 0;

	//LOG_ERROR(("yffs: compacting free space..."));
	--n;
	for(i = 0; i < n; ) {
		size_t j = i + 1;
		if (free[i].end == free[j].begin) {
			free[i].end = free[j].end;
			free[i].mtime = free[i].mtime > free[j].mtime? free[i].mtime: free[j].mtime;
			if (yffs_write_metadata(fs, free + i, 0, 0, 0) == -1)
				return -1;
			if (yffs_commit_metadata(fs, free + i) == -1)
				return -1;
			if (yffs_vector_remove(&fs->free, j, sizeof(struct yffs_block)) == -1)
				return -1;
			--n;
			free = (struct yffs_block *)fs->free.ptr; /*might be relocated*/
		} else
			++i;
	}
	return 0;
}

static int yffs_recover_and_remove_old_files(struct yffs *fs) {
	struct yffs_entry *files = (struct yffs_entry *)fs->files.ptr;
	size_t i, n = fs->files.size / sizeof(struct yffs_entry);
	if (n < 2)
		return 0;
	--n;
	uint32_t timestamp = (uint32_t)time(0);
	for(i = 0; i < n; ) {
		size_t j = i + 1;
		struct yffs_entry *file = files + i;
		if (strcmp(file->name, files[j].name) == 0) {
			file->block.mtime = timestamp;
			//LOG_ERROR(("yffs: unlinking older file %s@%u vs %u", file->name, (unsigned)file->block.mtime, (unsigned)files[j].block.mtime));
			if (yffs_write_metadata(fs, &file->block, 0, 0, 0) == -1)
				return -1;
			if (yffs_commit_metadata(fs, &file->block) == -1)
				return -1;
			free(file->name);
			if (yffs_vector_remove(&fs->files, i, sizeof(struct yffs_entry)) == -1)
				return -1;
			--n;
			files = (struct yffs_entry *)fs->files.ptr;
		} else
			++i;
	}
	return 0;
}

static int yffs_unlink_at(struct yffs *fs, size_t pos, int recursive) {
	struct yffs_entry *file = ((struct yffs_entry *)fs->files.ptr) + pos;
	struct yffs_block *free_block;
	
	//Check if file is a folder
	if(file->permBits >= 900 && recursive)
	{
		LOG_DEBUG(("Error cannot remove folder %s\n", file->name));
	}

	// LOG_DEBUG(("yffs: erasing metadata[%zu]:%s at 0x%zx-0x%zx", pos, file->name, file->block.begin, file->block.end));

	file->block.mtime = (uint32_t)time(0);
	if (yffs_write_metadata(fs, &file->block, 0, 0, 0) == -1)
		return -1;
	if (yffs_commit_metadata(fs, &file->block) == -1)
		return -1;
	
	free_block = (struct yffs_block *)yffs_vector_append(&fs->free, sizeof(struct yffs_block));
	*free_block = file->block;
	
	{
		size_t free_blocks = fs->free.size / sizeof(struct yffs_block);
		qsort(fs->free.ptr, free_blocks, sizeof(struct yffs_block), yffs_block_compare);
	}
	
	free(file->name);
	yffs_vector_remove(&fs->files, pos, sizeof(struct yffs_entry));
	return yffs_compact(fs);
}

int yffs_unlink(struct yffs *fs, const char *fname, int recursive) {
	long pos = yffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	return yffs_unlink_at(fs, pos, recursive);
}

static struct yffs_block *find_best_free(struct yffs *fs, size_t full_size) {
	struct yffs_block *free_begin = (struct yffs_block *)fs->free.ptr;
	struct yffs_block *free_end = (struct yffs_block *)(fs->free.ptr + fs->free.size);
	struct yffs_block *free, *best_free = 0;
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

size_t yffs_get_largest_free(struct yffs *fs) {
	struct yffs_block *free_begin = (struct yffs_block *)fs->free.ptr;
	struct yffs_block *free_end = (struct yffs_block *)(fs->free.ptr + fs->free.size);
	struct yffs_block *free;
	size_t largest = 0;

	for(free = free_begin; free < free_end; ++free) {
		size_t size = free->end - free->begin;
		if (size > largest)
			largest = size;
	}
	return largest;
}

size_t yffs_get_total_free(struct yffs *fs) {
	struct yffs_block *free_begin = (struct yffs_block *)fs->free.ptr;
	struct yffs_block *free_end = (struct yffs_block *)(fs->free.ptr + fs->free.size);
	struct yffs_block *free;
	size_t total = 0;

	for(free = free_begin; free < free_end; ++free) {
		total += free->end - free->begin;
	}
	return total;
}

ssize_t yffs_write(struct yffs *fs, const char *fname, const void *data, size_t size) {
	size_t fname_len = strlen(fname), best_size, full_size, tail_size;
	struct yffs_entry *file;
	long remove_me;
	struct yffs_block *best_free;
	off_t offset;
	uint8_t padding;

	long pos = yffs_find_file(fs, fname);
	if (pos < 0) {
		remove_me = -1;
		pos = -pos - 1;
	} else {
		remove_me = pos + 1;
		//LOG_ERROR(("yffs: old file in position %ld", remove_me));
	}
	file = (struct yffs_entry *)yffs_vector_insert(&fs->files, pos, sizeof(struct yffs_entry));
	if (!file) {
		//LOG_ERROR(("yffs: inserting file struct failed!"));
		return -1;
	}
	/*//LOG_DEBUG(("file[pos] = %s, file[pos + 1] = %s", ((struct yffs_entry *)fs->files.ptr)[pos].name, ((struct yffs_entry *)fs->files.ptr)[pos + 1].name));*/

	file->name = strdup(fname);
	char *user = (char *)malloc(sizeof(char)*10);
	if(getlogin_r(user, 10) != 0)
	  printf("problem getting user login...\n");
	file->owner = user;
	file->permBits = 14; //"rwr-" as default permissions settings
	printf("default permbits: %d\n", file->permBits);
	full_size = yffs_HEADER_SIZE + fname_len + size;
	best_free = find_best_free(fs, full_size);
	if (!best_free) {
		//LOG_ERROR(("yffs: no space left on device"));
		return -1;
	}

	best_size = best_free->end - best_free->begin;
	tail_size = best_size - full_size;
	offset = best_free->begin;
	if (tail_size > yffs_HEADER_SIZE) {
		/*mark up empty block inside*/
		best_free->begin = offset + full_size;
		if (yffs_write_empty_header(fs, best_free) == -1) 
			return -1;
		padding = 0;
	} else {
		/* tail_size == yffs_HEADER_SIZE or less */
		padding = tail_size;
		full_size += padding;
		yffs_vector_remove(&fs->free, best_free - (struct yffs_block *)fs->free.ptr, sizeof(struct yffs_block));
	}
	//LOG_ERROR(("yffs: creating file in position %ld -> 0x%lx", pos, (unsigned long)offset));
	file->block.begin = offset;
	file->block.end = offset + full_size;
	file->block.mtime = (uint32_t)time(0);
	file->padding = padding;
	file->size = size;

	/*writing data*/
	if (yffs_write_at(fs, offset + yffs_HEADER_SIZE + fname_len, data, size) == -1)
		return -1;
	/*writing metadata*/
	if (yffs_write_metadata(fs, &file->block, 0x40, fname_len, padding) == -1)
		return -1;
	/*write filename*/
	if (fs->write(fname, fname_len) != fname_len) {
		//LOG_ERROR(("yffs: error writing filename (len: %zu)", fname_len));
		return -1;
	}
	/*commit*/
	if (yffs_commit_metadata(fs, &file->block) == -1)
		return -1;

	if (remove_me >= 0) {
		/*removing old copy*/
		yffs_unlink_at(fs, remove_me, 0);
	}
	return size;
}

/*
gets the 
 */
ssize_t yffs_read(struct yffs *fs, const char *fname, void *data, size_t size) {
	struct yffs_entry *file;
	long pos = yffs_find_file(fs, fname), offset;

	if (pos < 0)
		return -1;

	file = ((struct yffs_entry *)fs->files.ptr) + pos;

	/*char *user = (char *)malloc(sizeof(char)*10);
	if(getlogin_r(user, 10) != 0)
	  printf("problem getting user login...\n");
	if(strcmp(user, file->owner) != 0) {
	  printf("user is not the owner of the file...\n");
	  return -1;
	  }*/

	if (size > file->size)
		size = file->size;
	
	offset = file->block.begin + yffs_HEADER_SIZE + strlen(fname);
	//LOG_ERROR(("yffs: reading from offset: %lu, size: %zu", (unsigned long)offset, size));
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(%ld, SEEK_SET) failed", offset));
		return -1;
	}
	return fs->read(data, size);
}

int yffs_stat(struct yffs *fs, const char *fname, struct stat *buf) {
	struct yffs_entry *file;
	long pos = yffs_find_file(fs, fname);
	if (pos < 0)
		return -1;

	memset(buf, 0, sizeof(*buf));
	file = ((struct yffs_entry *)fs->files.ptr) + pos;
	buf->st_size = file->size;
	buf->st_mtime = buf->st_ctime = file->block.mtime;
	return 0;
}

int yffs_format(struct yffs *fs) {
	struct yffs_block first_block;
	first_block.begin = 0;
	first_block.end = fs->device_size;
	return yffs_write_empty_header(fs, &first_block);
}


int yffs_mount(struct yffs *fs) {
	off_t offset, size = fs->seek(0, SEEK_END);

	if (size == (off_t)-1) {
		//LOG_ERROR(("yffs: cannot obtain device size: seek(0, SEEK_END) failed"));
		return -1;
	}

	//LOG_DEBUG(("yffs: device size: %zu bytes", size));
	fs->device_size = size;
	
	size = fs->seek(0, SEEK_SET);
	if (size == (off_t)-1) {
		//LOG_ERROR(("yffs: seek(0, SEEK_SET) failed"));
		return -1;
	}
	
	fs->files.ptr = 0;
	fs->files.size = 0;
	fs->free.ptr = 0;
	fs->free.size = 0;
	
	while((offset = fs->seek(0, SEEK_CUR)/* tell */) < fs->device_size) {
		struct yffs_block block;
		
		unsigned header_off, committed, block_size;
		uint8_t header[yffs_HEADER_SIZE];

		block.begin = offset;
		if (block.begin >= fs->device_size) {
			//LOG_ERROR(("yffs: block crosses device's bounds"));
			goto error;
		}
		
		if (fs->read(header, sizeof(header)) != sizeof(header)) {
			//LOG_ERROR(("yffs: failed reading block header"));
			goto error;
		}
		
		header_off = (header[0] & 0x80)? 5: 0;
		committed = header[header_off] & 0x40;

		block_size = le32toh(*(uint32_t *)(header + header_off + 1));
		if (block_size == 0) {
			//LOG_ERROR(("yffs: block size 0 is invalid"));
			goto error;
		}
		block.end = block.begin + yffs_HEADER_SIZE + block_size;
		block.mtime = (time_t)*(uint32_t *)&header[10];
		
		if (block.end == (off_t)-1) {
			//LOG_ERROR(("yffs: out of bounds"));
			goto error;
		}
		
		if (committed) {
			struct yffs_entry *file;
			size_t file_offset = fs->files.size;
			uint8_t filename_len = header[15], padding = header[14];

			if (filename_len == 0 || filename_len == 0xff)
				break;
		
			if (yffs_vector_resize(&fs->files, file_offset + sizeof(struct yffs_entry)) == -1)
				goto error;
			
			file = (struct yffs_entry *)((char *)fs->files.ptr + file_offset);
			file->name = malloc(filename_len + 1);
			if (!file->name) {
				//LOG_ERROR(("yffs: filename allocation failed(%u)", (unsigned)filename_len));
				goto error;
			}
			if (fs->read(file->name, filename_len) != filename_len) {
				//LOG_ERROR(("yffs: read(%u) failed", (unsigned)filename_len));
				goto error;
			}
			file->size = block_size - filename_len - padding;
			file->name[filename_len] = 0;

			// LOG_DEBUG(("yffs: read file %s -> %zu", file->name, file->size));

			file->block = block;
		} else {
			struct yffs_block *free;
			size_t free_offset = fs->free.size;
			if (yffs_vector_resize(&fs->free, free_offset + sizeof(struct yffs_block)) == -1)
				goto error;
			free = (struct yffs_block *)((char *)fs->free.ptr + free_offset);
			*free = block;

			// LOG_DEBUG(("yffs: free space %zu->%zu", block.begin, block.end));

			if (free->end > fs->device_size)  {
				//LOG_ERROR(("yffs: free spaces crosses device bound!"));
				free->end = fs->device_size;
				goto error;
			}
		}
		if (fs->seek(block.end, SEEK_SET) == (off_t)-1) {
			//LOG_ERROR(("yffs: block end %zu is out of bounds", block.end));
			goto error;
		}
	}
	if (offset > fs->device_size) {
		//LOG_ERROR(("yffs: last block crosses device bounds"));
		goto error;
	}
	if (fs->files.size == 0 && fs->free.size == 0) {
		//LOG_ERROR(("yffs: corrupted file system: no free blocks or files."));
		return -1; //no need to cleanup
	}
	
	{
		size_t files = fs->files.size / sizeof(struct yffs_entry);
		qsort(fs->files.ptr, files, sizeof(struct yffs_entry), yffs_entry_compare);
	}

	if (yffs_recover_and_remove_old_files(fs) == -1)
		goto error;

	if (yffs_compact(fs) == -1)
		goto error;
	
	// LOG_DEBUG(("yffs: mounted!"));

	return 0;
	
error:
	//do some cleanups:
	yffs_umount(fs);
	return -1;
}


int yffs_umount(struct yffs *fs) {
	free(fs->files.ptr);
	fs->files.ptr = 0;
	fs->files.size = 0;
	free(fs->free.ptr);
	fs->free.ptr = 0;
	fs->free.size = 0;
	return 0;
}

void encrypt(char * fname, int mode) {
        if (testing) {return;}
        int i = 0;
        while (fname[i]) {
            fname[i++]++;
        }
        return;
}

void decrypt(char * fname, int mode) {
        if (testing) {return;}
        int i = 0;
        while (fname[i]) {
            fname[i++]--;
        }
}
