//=========================================================================
//                      ilib Arean Allocator
//=========================================================================
// by      : INSANE
// created : 22/10/2026
//
// purpose : Simple single-header arena-allocator
//-------------------------------------------------------------------------
#include "ilib_arena_alloc.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>


#define NO_DISCARD __attribute__((warn_unused_result))


NO_DISCARD static int pushback_block(struct ilib_arena *a);
NO_DISCARD static void* alloc_mem(enum iaa_allocator_type allocator, size_t nbytes);
           static void free_mem(void *p, size_t nbytes, enum iaa_allocator_type allocator);
NO_DISCARD static size_t align_llu(size_t x, size_t align);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int iaa_init(struct ilib_arena *a)
{
	return iaa_init_ex(a, UINT32_MAX, sysconf(_SC_PAGESIZE), ILIB_ARENA_MMAP);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int iaa_init_ex(struct ilib_arena *a, uint32_t max_blocks, size_t block_size, enum iaa_allocator_type allocator_type)
{
	a->allocator = allocator_type;
	a->block_size = block_size;
	a->max_blocks = max_blocks;
	a->block_count = 0;
	a->blocks = NULL;
	a->active_block_index = 0;
	a->pointer = 0;

	return pushback_block(a);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void* iaa_alloc(struct ilib_arena *a, size_t nbytes)
{
	return iaa_alloc_ex(a, nbytes, DEFAULT_ALIGNMENT);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void* iaa_alloc_ex(struct ilib_arena *a, size_t nbytes, size_t align)
{
 	assert(a->block_count >= 1 && "Atleast one block must be allocated");
	assert(nbytes + align <= a->block_size && "allocation size + alignment requirement must be <= block_size");
	assert(nbytes > 0 && "allocation size must be non-zero");
	if (a->block_count <= 0 || nbytes <= 0 || nbytes + align > a->block_size)
		return NULL;

	// algo-start.
	// NOTE : nbytes + align <= a->block_size. (align_llu(x, align) - x) is always < align. 
	void *active_block = a->blocks[a->active_block_index];
	size_t i = (size_t)active_block + a->pointer;
	if (a->pointer + (align_llu(i, align) - i) + nbytes > a->block_size) {
		a->pointer = 0;
		a->active_block_index += 1;

		if (a->active_block_index >= a->block_count)
			if (pushback_block(a) != 0)
				return NULL;
	}

	i = (size_t)(a->blocks[a->active_block_index]) + a->pointer;
	a->pointer += (align_llu(i, align) - i) + nbytes;

	return (void*)align_llu(i, align);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void iaa_unalloc(struct ilib_arena *a, size_t nbytes)
{
	size_t pointer_global = a->active_block_index * a->block_size + a->pointer;
	assert(pointer_global >= nbytes && "Trying to free more than size");

	pointer_global -= nbytes;
	a->active_block_index = (pointer_global / a->block_size);
	a->pointer = pointer_global % a->block_size;
	return;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void iaa_free(struct ilib_arena *a)
{
	for (size_t i = 0; i < a->block_size; ++i) {
		free_mem(a->blocks[i], a->block_size, a->allocator);
	}
	free(a->blocks);
	a->blocks = NULL;
	a->block_count = 0;
	a->active_block_index = 0;
	a->pointer = 0;
	return;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
size_t iaa_capacity(struct ilib_arena *a)
{
	return a->block_count * a->block_size;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
size_t iaa_bytes_used(struct ilib_arena *a)
{
	return a->active_block_index * a->block_size + a->pointer;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
NO_DISCARD static size_t align_llu(size_t x, size_t align)
{
	if (align == 0)
		return x;

	if ((x % align) == 0)
		return x;

	return x - (x % align) + align;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
NO_DISCARD static int pushback_block(struct ilib_arena *a)
{
	size_t new_block_count = a->block_count + 1;
	void *tmp = realloc(a->blocks, sizeof(void*) * new_block_count);
	if (!tmp)
		return 1;
	a->blocks = tmp;

	void *new_block = alloc_mem(a->allocator, a->block_size);
	if (!new_block)
		return 1;

	a->block_count = new_block_count;
	a->blocks[new_block_count - 1] = new_block;
	return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
NO_DISCARD static void* alloc_mem(enum iaa_allocator_type allocator, size_t nbytes)
{
	switch (allocator) {
	case ILIB_ARENA_MALLOC: return malloc(nbytes);
	case ILIB_ARENA_MMAP: return mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	default: break;
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void free_mem(void *p, size_t nbytes, enum iaa_allocator_type allocator)
{
	switch (allocator) {
	case ILIB_ARENA_MALLOC: free(p); return;
	case ILIB_ARENA_MMAP: munmap(p, nbytes); return;
	}

	assert(false && "Invalid allocator detected in arena struct");
}
