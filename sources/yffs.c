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
	
	//Return files in directory
	if (index < fs->files.size) {
		struct yffs_entry * ent = (struct yffs_entry *)(fs->files.ptr + index);
		//printf("Ent\nDir->%s\nFile->%s\n", ent->dir, ent->name);

		//If this is in the same directory print out the file
		if(ent->dir && strcmp(ent->dir, directory) == 0 ){
			//printf("Name is %s\n", ent->name);
			return ent->name;
		}
		else{
			//Or return directories
			//printf("Name is %s\n", ent->name);
			if(ent->dir){
				//printf("Dir is %s\n", ent->dir);
				if(strstr(ent->dir, directory) != NULL) {
					return ent->dir;
				}
				else{
					//printf("Returning null\n");
					return "";
				}
			}
			else{
				//Shouldn't happen

				//printf("Dir is null\n");
				//char * fname = ent->name;
				// memmove (fname, fname+1, strlen (fname+1) + 1);
				return ent->name;
			}
		}
	} else
		return 0;
}


static int yffs_write_empty_header(struct yffs *fs, struct yffs_block *block) {
	size_t size = block->end - block->begin;
	if (size <= yffs_HEADER_SIZE) {
		//LOG_ERROR(("yffs: avoid writing block <= 16b."));
		return -1;
	}
	char header[yffs_HEADER_SIZE] = { //header indexes used 0, 1, 10, 11, 12 (owner_len), 13 (permbyte), 14 (dir_len)
		0, 0, 0, 0, 0, /*1:flags(empty) + size, current 1st*/
		0, 0, 0, 0, 0, /*2:flags + size*/
		0, 0, 0, 0, 0, 0, /*mtime + padding + dir_len + fname len */
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

static int yffs_write_metadata(struct yffs *fs, struct yffs_block *block, uint8_t flags, uint8_t fname_len, uint8_t dir_len, uint8_t owner_len, uint8_t padding, char perm) {
	uint8_t header[10], header_offset;
	uint8_t header2[4 + 1 + 1]; /*timestamp + dir_len + filename len */ //NOTICE: i did not do anything for owner_len here

	
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
	header2[2] = owner_len;
	header2[4] = dir_len;
	header2[5] = fname_len;
	header2[3] = perm;

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
		{
			//printf("Returning %d\n", mid);
			return mid;
		}
	}
	//printf("Returning %d", first +1);
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
			if (yffs_write_metadata(fs, free + i, 0, 0, 0, 0, 0, 0) == -1)
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

//gets the permission bits at 'index'
const char yffs_permission(struct yffs *fs, const char *filename) {
	struct yffs_entry *file;
	long pos = yffs_find_file(fs, filename), offset;

	if (pos < 0)
		return 0;

	file = ((struct yffs_entry *)fs->files.ptr) + pos;

	int dir_len = strlen(file->dir);
	int owner_len = strlen(file->owner);

	offset = file->block.begin; // + yffs_HEADER_SIZE + strlen(filename) + dir_len + owner_len;
	if (fs->seek(offset, SEEK_SET) == (off_t)-1) {
		return 0;
	}

	uint8_t header[yffs_HEADER_SIZE];

	
	if (fs->read(header, sizeof(header)) != sizeof(header)) {
		//LOG_ERROR(("yffs: failed reading block header"));
		return 0;
	}
	
	return header[13];
}

//gets the owner at 'index'
const char *yffs_owner(struct yffs *fs, const char *filename) {
	long index = yffs_find_file(fs, filename);
	index *= sizeof(struct yffs_entry);
	if (index < fs->files.size) {
		return ((struct yffs_entry *)(fs->files.ptr + index))->owner;
	} else
		return 0;
}

//returns 1 if user has write permissions, 0 otherwise
int have_write(struct yffs *fs, const char *filename) {
	char permbit = yffs_permission(fs, filename);
	char *user = (char *)malloc(sizeof(char)*10);
	if(getlogin_r(user, 10) != 0)
	  printf("problem getting user login...\n");

	char *owner = yffs_owner(fs, filename);

	if(strcmp(owner, user) == 0) {
		//printf("you are the owner\n");
		permbit = permbit & 4;
		if(permbit != 0)
			return 1;
		else
			return 0;
	}
	else {
		//printf("you are NOT the owner\n");
		permbit = permbit & 1;
		if(permbit != 0)
			return 1;
		else
			return 0;
	}
}

//returns 1 if user has read permissions, 0 otherwise
int have_read(struct yffs *fs, const char *filename) {
	char permbit = yffs_permission(fs, filename);
	char *user = (char *)malloc(sizeof(char)*10);
	if(getlogin_r(user, 10) != 0)
	  printf("problem getting user login...\n");

	char *owner = yffs_owner(fs, filename);

	if(strcmp(owner, user) == 0) {
		//printf("you are the owner\n");
		permbit = permbit & 8;
		if(permbit != 0)
			return 1;
		else
			return 0;
	}
	else {
		//printf("you are NOT the owner\n");
		permbit = permbit & 2;
		if(permbit != 0)
			return 1;
		else
			return 0;
	}
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
			LOG_ERROR(("yffs: unlinking older file %s@%u vs %u", file->name, (unsigned)file->block.mtime, (unsigned)files[j].block.mtime));
			if (yffs_write_metadata(fs, &file->block, 0, 0, 0, 0, 0, 0) == -1)
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
	if (yffs_write_metadata(fs, &file->block, 0, 0, 0, 0, 0, 0) == -1)
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

	//Get the file without any slashes
	int index = strlstchar(fname, '/');

	size_t fname_len, dir_len, owner_len, best_size, full_size, tail_size;
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
	//printf("Pre substring\n");
	//Insert the rest of the directory into dir attribute in yffs_entry
	char * directory = (char*)substring(fname, 0, index+1);
	if(index == -1){
		//printf("No folder given\n");
		char * buff = "/";
		//printf("Strlen is %d\n", 1);
		file->dir = (char *)malloc(1 * sizeof(char));
		file->dir = buff;
		dir_len = 1;
	}
	else{
		file->dir = (char *)malloc(strlen(directory) * sizeof(char));
		file->dir = directory;
		dir_len = strlen(directory);
	}
	//printf("Dir is %s\n", directory);
	//printf("Set to %s\n", file->dir);

	//Set the name into file
	file->name = (char *)malloc((strlen(fname) - (index+1)) *sizeof(char));
	file->name = (char *)substring(fname, index+1, strlen(fname) - (index+1));

	//printf("File is %s\n", file->name);

	fname_len = strlen(file->name);

	char *user = (char *)malloc(sizeof(char)*10);
	if(getlogin_r(user, 10) != 0)
	  printf("problem getting user login...\n");
	file->owner = user;
	//file->permBits = 14; //"rwr-" as default permissions settings
	owner_len = strlen(file->owner);

	full_size = yffs_HEADER_SIZE + dir_len + owner_len + fname_len + size;
	best_free = find_best_free(fs, full_size);
	if (!best_free) {
		LOG_ERROR(("yffs: no space left on device"));
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
	if (yffs_write_at(fs, offset + yffs_HEADER_SIZE + fname_len + dir_len + owner_len, data, size) == -1)
		return -1;
	/*writing metadata*/
	if (yffs_write_metadata(fs, &file->block, 0x40, fname_len, dir_len, owner_len, padding, 14) == -1)
		return -1;
	/*write owner*/ 
	if(fs->write(file->owner, owner_len) != owner_len)
	{
		//LOG_ERROR(("yffs: error writing owner (len: %zu)", dir_len));
		return -1;
	}
	
	/*write dirname*/
	if(fs->write(file->dir, dir_len) != dir_len)
	{
		//LOG_ERROR(("yffs: error writing directory (len: %zu)", dir_len));
		return -1;
	}
	/*write filename*/
	if (fs->write(file->name, fname_len) != fname_len) {
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

//same as write, but with defined permissions
ssize_t yffs_write_perm(struct yffs *fs, const char *fname, const void *data, size_t size, char perm) {

	//Get the file without any slashes
	int index = strlstchar(fname, '/');

	size_t fname_len, dir_len, owner_len, best_size, full_size, tail_size;
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
	//printf("Pre substring\n");
	//Insert the rest of the directory into dir attribute in yffs_entry
	char * directory = (char*)substring(fname, 0, index+1);
	if(index == -1){
		//printf("No folder given\n");
		char * buff = "/";
		//printf("Strlen is %d\n", 1);
		file->dir = (char *)malloc(1 * sizeof(char));
		file->dir = buff;
		dir_len = 1;
	}
	else{
		file->dir = (char *)malloc(strlen(directory) * sizeof(char));
		file->dir = directory;
		dir_len = strlen(directory);
	}
	//printf("Dir is %s\n", directory);
	//printf("Set to %s\n", file->dir);

	//Set the name into file
	file->name = (char *)malloc((strlen(fname) - (index+1)) *sizeof(char));
	file->name = (char *)substring(fname, index+1, strlen(fname) - (index+1));

	//printf("File is %s\n", file->name);

	fname_len = strlen(file->name);

	file->owner = yffs_owner(fs, file->name);
	//file->permBits = 14; //"rwr-" as default permissions settings
	owner_len = strlen(file->owner);

	full_size = yffs_HEADER_SIZE + dir_len + owner_len + fname_len + size;
	best_free = find_best_free(fs, full_size);
	if (!best_free) {
		LOG_ERROR(("yffs: no space left on device"));
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
	if (yffs_write_at(fs, offset + yffs_HEADER_SIZE + fname_len + dir_len + owner_len, data, size) == -1)
		return -1;
	/*writing metadata*/
	if (yffs_write_metadata(fs, &file->block, 0x40, fname_len, dir_len, owner_len, padding, perm) == -1)
		return -1;
	/*write owner*/ 
	if(fs->write(file->owner, owner_len) != owner_len)
	{
		//LOG_ERROR(("yffs: error writing owner (len: %zu)", dir_len));
		return -1;
	}
	
	/*write dirname*/
	if(fs->write(file->dir, dir_len) != dir_len)
	{
		//LOG_ERROR(("yffs: error writing directory (len: %zu)", dir_len));
		return -1;
	}
	/*write filename*/
	if (fs->write(file->name, fname_len) != fname_len) {
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


//same as yffs_write with the owner specified
ssize_t yffs_write_own(struct yffs *fs, const char *fname, const void *data, size_t size, char *owner) {

	//Get the file without any slashes
	int index = strlstchar(fname, '/');

	size_t fname_len, dir_len, owner_len, best_size, full_size, tail_size;
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
	//printf("Pre substring\n");
	//Insert the rest of the directory into dir attribute in yffs_entry
	char * directory = (char*)substring(fname, 0, index+1);
	if(index == -1){
		//printf("No folder given\n");
		char * buff = "/";
		//printf("Strlen is %d\n", 1);
		file->dir = (char *)malloc(1 * sizeof(char));
		file->dir = buff;
		dir_len = 1;
	}
	else{
		file->dir = (char *)malloc(strlen(directory) * sizeof(char));
		file->dir = directory;
		dir_len = strlen(directory);
	}
	//printf("Dir is %s\n", directory);
	//printf("Set to %s\n", file->dir);

	//Set the name into file
	file->name = (char *)malloc((strlen(fname) - (index+1)) *sizeof(char));
	file->name = (char *)substring(fname, index+1, strlen(fname) - (index+1));

	//printf("File is %s\n", file->name);

	fname_len = strlen(file->name);

	/*char *user = (char *)malloc(sizeof(char)*10);
	if(getlogin_r(user, 10) != 0)
	  printf("problem getting user login...\n");*/
	file->owner = owner;
	owner_len = strlen(owner);

	//file->permBits = 14; //"rwr-" as default permissions settings

	full_size = yffs_HEADER_SIZE + dir_len + owner_len + fname_len + size;
	best_free = find_best_free(fs, full_size);
	if (!best_free) {
		LOG_ERROR(("yffs: no space left on device"));
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
	if (yffs_write_at(fs, offset + yffs_HEADER_SIZE + fname_len + dir_len + owner_len, data, size) == -1)
		return -1;
	/*writing metadata*/
	if (yffs_write_metadata(fs, &file->block, 0x40, fname_len, dir_len, owner_len, padding, yffs_permission(fs, fname)) == -1)
		return -1;
	/*write owner*/ 
	if(fs->write(file->owner, owner_len) != owner_len)
	{
		//LOG_ERROR(("yffs: error writing owner (len: %zu)", dir_len));
		return -1;
	}
	
	/*write dirname*/
	if(fs->write(file->dir, dir_len) != dir_len)
	{
		//LOG_ERROR(("yffs: error writing directory (len: %zu)", dir_len));
		return -1;
	}
	/*write filename*/
	if (fs->write(file->name, fname_len) != fname_len) {
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
ssize_t yffs_read(struct yffs *fs, const char * directory, const char *fname, void *data, size_t size) {
	struct yffs_entry *file;
	long pos = yffs_find_file(fs, fname), offset;

	if (pos < 0)
		return -1;

	file = ((struct yffs_entry *)fs->files.ptr) + pos;

	//Check if file matches with folder
	if(strcmp(file->dir, directory) != 0)
	{
		printf("Error wrong file\n");
		return -1;
	}

	int dir_len = strlen(file->dir);
	int owner_len = strlen(file->owner);

	//printf("Dir lent is %d\n", dir_len);

	if (size > file->size)
		size = file->size;
	
	offset = file->block.begin + yffs_HEADER_SIZE + strlen(fname) + dir_len + owner_len;
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
	//printf("File %s size is %d\n", file->name, file->size);
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
			uint8_t dir_len = header[14];
			uint8_t owner_len = header[12];

			if (filename_len == 0 || filename_len == 0xff)
				break;

			if (dir_len == 0 || dir_len == 0xff)
				break;
	
			if (owner_len == 0 || owner_len == 0xff)
				break;
			
			if (yffs_vector_resize(&fs->files, file_offset + sizeof(struct yffs_entry)) == -1)
				goto error;
			
			file = (struct yffs_entry *)((char *)fs->files.ptr + file_offset);
			file->name = malloc(filename_len + 1);
			file->dir = malloc(dir_len + 1);
			file->owner = malloc(owner_len + 1);

			if(!file->dir)
			{
				goto error;
			}

			if (!file->name) {
				//LOG_ERROR(("yffs: filename allocation failed(%u)", (unsigned)filename_len));
				goto error;
			}

			if (!file->owner) {
				//LOG_ERROR(("yffs: filename allocation failed(%u)", (unsigned)filename_len));
				goto error;
			}

			if (fs->read(file->owner, owner_len) != owner_len) {
				//LOG_ERROR(("yffs: read(%u) failed", (unsigned)filename_len));
				goto error;
			}


			if (fs->read(file->dir, dir_len) != dir_len) {
				//LOG_ERROR(("yffs: read(%u) failed", (unsigned)filename_len));
				goto error;
			}

			if (fs->read(file->name, filename_len) != filename_len) {
				//LOG_ERROR(("yffs: read(%u) failed", (unsigned)filename_len));
				goto error;
			}
			file->size = block_size - filename_len - dir_len - owner_len; // - padding; //NOTICE: i did not do anything for owner_len here
			file->name[filename_len] = 0;
			file->dir[dir_len] = 0;
			file->owner[owner_len] = 0;

			// LOG_DEBUG(("yffs: read file %s -> %zu", file->name, file->size));

			file->block = block;
		} else {
			struct yffs_block *free;
			size_t free_offset = fs->free.size;
			if (yffs_vector_resize(&fs->free, free_offset + sizeof(struct yffs_block)) == -1)
				goto error;
			free = (struct yffs_block *)((char *)fs->free.ptr + free_offset);
			*free = block;

			//LOG_DEBUG(("yffs: free space %zu->%zu", block.begin, block.end));

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

void encrypt_file(char * fname, char * argv) {
        if (testing) {return;}
        int i = 0;
        while (fname[i]) {
            fname[i++]++;
        }
}

void decrypt_file(char * fname, char * argv) {
        if (testing) {return;}
        int i = 0;
        while (fname[i]) {
            fname[i++]++;
        }
}

char * substring(char *string, int position, int length) 
{
   char pointer[length+1];

   memset(&pointer[0], 0, sizeof(pointer));
   int c;
 
   for (c = 0 ; c < length ; c++)
   {
      pointer[c] = *(string+position);      
      string++;   
   }
 
   pointer[c] = '\0';
 
   return strdup(pointer);
}

size_t strlstchar(const char *str, const char ch)
{
    char *chptr = strrchr(str, ch);
    return (chptr != NULL) ? chptr - str : -1;
}
