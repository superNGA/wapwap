//=========================================================================
//                      ILIB Vector
//=========================================================================
// by      : INSANE
// created : 22/02/2026
//
// purpose : std::vector<> rip-off in C. Inspired by nothings/stb_ds.h 
//-------------------------------------------------------------------------
#ifndef ILIB_VECTOR_H
#define ILIB_VECTOR_H


/*

Vector Structure : 
    [ Header ][ Entry 0 ][ Entry 1 ][ Entry 2 ]...

*/


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>


#define ILIB_VEC_DEFAULT_CAPCITY (10)
#define ILIB_VEC_ARBITRARY_SIGNATURE (0xBED0DECA)
#define ILIB_VEC_GROWTH_RATE (2)



// Get pointer to ilib_vec_header stored before the 0th element (look at the vector structure at file beginning).
#define vec_get_header(arr) (&((struct ilib_vec_header*)arr)[-1])

#define vec_len(arr) ((arr) == NULL ? 0 : vec_get_header(arr)->size)

#define vec_capacity(arr) ((arr) == NULL ? 0 : vec_get_header(arr)->capacity)

// 0 -> empty. else not empty
#define vec_empty(arr) (!vec_len(arr))

// pointer to last element in vector
#define vec_back(arr) (&(arr)[vec_len(arr) - 1])

// pointer to first element in vector
#define vec_front(arr) (&(arr)[0])

// Insert element at any index (shallow copy at index)
#define vec_insert(arr, index, Data) do { \
	void **p_arr = (void**)&(arr); \
	size_t elem_size = sizeof(*(arr)); \
	if (arr == NULL) \
		ilib_vec_init(p_arr, elem_size); \
	struct ilib_vec_header *hdr = vec_get_header(arr); \
	int new_size = (index) <= hdr->size ? hdr->size + 1 : (index) + 1; \
	if (new_size > hdr->capacity) { \
		ilib_vec_grow(p_arr, elem_size, new_size); \
		hdr = vec_get_header(arr); \
	} \
	if ((index) < hdr->size) \
		memmove((arr) + (index) + 1, (arr) + (index), elem_size * (hdr->size - (index))); \
	(arr)[(index)] = (Data); \
	hdr->size = new_size; \
} while(0)

// Erase element at any index
#define vec_erase(arr, index) do { \
	assert((arr) != NULL && "Cannot erase from an uninitialized array"); \
	struct ilib_vec_header *hdr = vec_get_header((arr)); \
	assert(index < hdr->size && index >= 0 && "erase index out of range");\
	if ((index) < hdr->size - 1) { \
		size_t elem_size = sizeof(*(arr)); \
		memmove((arr) + (index), (arr) + (index) + 1, elem_size * (hdr->size - (index) - 1)); \
	} \
	hdr->size -= 1; \
} while(0)

#define vec_pushback(arr, data) vec_insert((arr), vec_len(arr), data)
#define vec_popback(arr) vec_erase((arr), vec_len(arr) - 1)

#define vec_free(arr) do{ \
	assert((arr) != NULL && "can't free an uninitialized array"); \
	free(vec_get_header(arr)); \
	(arr) = NULL; \
} while(0)

// Set element count as 0 for this vector.
#define vec_clear(arr) do{ \
	if (arr != NULL) { \
		struct ilib_vec_header *hdr = vec_get_header(arr); \
		hdr->size = 0; \
	} \
} while(0)

#define vec_reserve(arr, size) do{ \
	void **arr_out = (void**)&(arr); \
	size_t elem_size = sizeof(*(arr)); \
	if ((arr) == NULL) \
		ilib_vec_init(arr_out, elem_size); \
	struct ilib_vec_header *hdr = vec_get_header(arr); \
	if (size > hdr->capacity) \
		ilib_vec_grow(arr_out, elem_size, size); \
} while(0)



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct ilib_vec_header
{
    unsigned int size;         // number of elements
    unsigned int capacity;     // malloced_size / element size (excluding the header)
    unsigned int element_size; // sizeof(element)
    unsigned int signature;    // Signature (random ass number) to help prevent mistakes.
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void ilib_vec_init(void** arr_out, size_t elem_size)
{
	assert(*arr_out == NULL && "Trying to initialize non-null array");

	size_t default_size  = sizeof(struct ilib_vec_header) + (elem_size * ILIB_VEC_DEFAULT_CAPCITY);
	struct ilib_vec_header* hdr = (struct ilib_vec_header*)malloc(default_size);
	memset(hdr, 0, default_size);

	hdr->element_size = elem_size;
	hdr->signature    = ILIB_VEC_ARBITRARY_SIGNATURE;
	hdr->capacity     = ILIB_VEC_DEFAULT_CAPCITY;
	hdr->size         = 0;

	// array_out should point at first element and not at the header.
	*arr_out = hdr + 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void ilib_vec_grow(void** arr_out, size_t elem_size, size_t new_size)
{
	struct ilib_vec_header* hdr = vec_get_header(*arr_out);

	assert(hdr->capacity > 0 && "invalid array received");
	assert(new_size > hdr->capacity && "Trying to grow an already big enough vector");
	if (new_size <= hdr->capacity)
		return;


	// old_capacity * growth_rate^n = new_size
	// growth_rate^n = new_size / old_capacity
	// log(growth_rate^n) = log(new_size / old_capacity)
	// n * log(growth_rate) = log(new_size / old_capacity)
	// n = log(new_size / old_capacity) / log(growth_rate)
	// float old_capacity = (float)hdr->capacity;
	// float growth_power = log((float)new_size / old_capacity) / log((float)ILIB_VEC_GROWTH_RATE);
	// float new_capacity = old_capacity * pow((float)ILIB_VEC_GROWTH_RATE, ceil(growth_power));
	
	// Justification : If new size is significantly more than old capacity, then it is obviously not 
	// following the +1 pattern, in which case, constant growth rate approach fails to deliver its 
	// intended performance benefits. Therefore, we would be better resizing to the new unexpected size.
	unsigned int new_capc = new_size >= hdr->capacity * 2 ? new_size : hdr->capacity * 2;

	unsigned int safe_cap = new_capc + 1;
	size_t safe_size = sizeof(struct ilib_vec_header) + elem_size * safe_cap;
	assert(safe_cap > 0 && safe_cap >= new_size && "New capacity calculation has failed");
	struct ilib_vec_header* new_hdr = (struct ilib_vec_header*)realloc(hdr, safe_size);
	new_hdr->capacity = safe_cap;

	*arr_out = new_hdr + 1;
}


#endif // ILIB_VECTOR_H
