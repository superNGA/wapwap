//=========================================================================
//                      ilib Arean Allocator
//=========================================================================
// by      : INSANE
// created : 22/10/2026
//
// purpose : Simple single-header arena-allocator
//-------------------------------------------------------------------------
#ifndef ILIB_ARENA_ALLOC_H
#define ILIB_ARENA_ALLOC_H


#include <stddef.h>
#include <stdint.h>


#define DEFAULT_ALIGNMENT (8)



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
enum iaa_allocator_type {
	ILIB_ARENA_MALLOC,
	ILIB_ARENA_MMAP,
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct ilib_arena {
	enum iaa_allocator_type allocator;
	size_t block_size; 

	uint32_t max_blocks;
	uint32_t block_count;
	void** blocks;

	size_t active_block_index;
	size_t pointer;
};


/*
 * Wrapper for iaa_init_ex.
 * Initializes a->max_arenas to UINT32_MAX
 * Initializes a->block_size to sysconf(_SC_PAGESIZE)
 * Initializes a->allocator_type to ILIB_ARENA_MMAP
 *
 * iaa_init_ex(a, UINT32_MAX, sysconf(_SC_PAGESIZE), ILIB_ARENA_MMAP)
 *
 * return 0 on success, !0 on failure
 */
int iaa_init(struct ilib_arena *a);

/*
 * The constructor.
 *
 * `a` is a caller owned instance of struct ilib_arena. Although the struct is not opaque, 
 * direct modifications to the member variables of `a` are highly discouraged.
 *
 * return 0 on success, !0 on failure
 */
int iaa_init_ex(struct ilib_arena *a, uint32_t max_blocks, size_t block_size, enum iaa_allocator_type allocator_type);

/*
 * Wrapper for iaa_alloc_ex. 
 * Allocates memory with an alignment of DEFAULT_ALIGNMENT.
 * iaa_alloc_ex(arena, nbytes, DEFAULT_ALIGNMENT)
 */
void* iaa_alloc(struct ilib_arena *a, size_t nbytes);

/*
 * Allocates `nbytes` number of bytes with an alignment of `align` from the arena index = (arena_pointer / arena_block_size).
 *
 * Incase the target arena-block does not exist (incase of first alloc) or cannot accomodate `nbytes` bytes
 * a new arena-block of size `a->block_size` is allocated using the allocator type defined in the arena struct, 
 * and stored.
 *
 * Incase the `nbytes` is greater than block_size defined in the arena struct, NULL is returned.
 *
 * If allocation succeeds a valid pointer to the allocated memory is returned, and the `a->pointer` is 
 * moved forward accordingly.
 */
void* iaa_alloc_ex(struct ilib_arena *a, size_t nbytes, size_t align);

/*
 * Simply performs `a->pointer` -= bytes. This moves the arena pointer backwards, which causes
 * any further allocations to overwrite memory at the new pointer location.
 */
void iaa_unalloc(struct ilib_arena *a, size_t nbytes);

/*
 * Unallocates all arena-blocks that belong to this arena using the allocator type 
 * specified in the arena struct.
 */
void iaa_free(struct ilib_arena *a);

/*
 * Total arena-blocks allocated * size of one arena-block.
 */
size_t iaa_capacity(struct ilib_arena *a);

/*
 * Number of bytes used across all arena-blocks (represented by `a->pointer`).
 */
size_t iaa_bytes_used(struct ilib_arena *a);


#endif // ILIB_ARENA_ALLOC_H
