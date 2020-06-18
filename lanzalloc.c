#include "lanzalloc.h"

typedef unsigned char byte_t;

typedef struct {
	lanzalloc_size_t size; // If size == 0, it is a unused unit.
	void* address; // If address == 0, it is a free unit.
} unit_t;

typedef struct {
	lanzalloc_size_t size_info;
	byte_t bytes[0];
} pool_block_t;

typedef struct lanzalloc {
	void* pool;
	lanzalloc_size_t pool_size;
	lanzalloc_size_t unit_max;
	unit_t* units;
} lanzalloc_t;

#define UNIT_USED(unit)						( (unit)->size != 0 )
#define UNIT_UNUSED(unit)					( (unit)->size == 0 )

#define UNIT_SET(unit, a, s)				{ (unit)->size = (s); (unit)->address = (a); }
#define UNIT_RELEASE(unit)					{ (unit)->size = 0; (unit)->address = 0; }

static int lanzalloc_defragment(struct lanzalloc* lanzalloc) {
	lanzalloc_size_t i, j, maxunit = lanzalloc->unit_max;
	for ( i = 0; i < maxunit; i++ ) {
		unit_t* unit = lanzalloc->units + i;
		if ( UNIT_USED(unit) ) {
			void* tail = (void*)((byte_t*)unit->address + unit->size);
			for ( j = 0; j < maxunit; j++ ) {
				unit_t* unit2 = lanzalloc->units + j;
				if (unit != unit2 && unit2->size && tail == unit2->address) {
					unit->size += unit2->size;
					UNIT_RELEASE(unit2);
					return 1;
				}
			}
		}
	}
	return 0;
}

static void lanzalloc_defragment_full_cycle(struct lanzalloc* lanzalloc) {
	while(lanzalloc_defragment(lanzalloc));
}

struct lanzalloc* lanzalloc_initialize(void* memory, lanzalloc_size_t size, lanzalloc_size_t maxunit) {
	struct lanzalloc* lanzalloc = (struct lanzalloc*)memory;
	if (size < (sizeof(lanzalloc_t) + sizeof(unit_t) * maxunit)) return (lanzalloc_t*)0;
	lanzalloc->unit_max = maxunit;
	lanzalloc->units = (unit_t*)((byte_t*)memory + sizeof(lanzalloc_t));
	lanzalloc->pool = (void*)((byte_t*)memory + sizeof(lanzalloc_t) + sizeof(unit_t) * maxunit);
	lanzalloc->pool_size = size - (sizeof(lanzalloc_t) + sizeof(unit_t) * maxunit);
	lanzalloc_size_t i = 1;
	for (; i < maxunit; i++) UNIT_RELEASE(lanzalloc->units + i);
	UNIT_SET(lanzalloc->units, lanzalloc->pool, lanzalloc->pool_size);
	return lanzalloc;
}

void* lanzalloc_alloc(struct lanzalloc* lanzalloc, lanzalloc_size_t size) {
	lanzalloc_size_t i, maxunit = lanzalloc->unit_max;
	do {
		for (i = 0; i < maxunit; i++) {
			unit_t* unit = lanzalloc->units + i;
			// Find a unit.
			if ( UNIT_USED(unit) ) {
				lanzalloc_size_t block_size = size + sizeof(pool_block_t);
				if ( unit->size >= block_size ) {
					pool_block_t* block = (pool_block_t*)unit->address;
					block->size_info = size;
					if ( unit->size > block_size ) {
						UNIT_SET(unit, unit->address, unit->size - block_size);
					} else {
						UNIT_RELEASE(unit);
					}
					// Return pointer.
					return (void*)block->bytes;
				}
			}
		}
	} while(lanzalloc_defragment(lanzalloc));
	return 0;
}

void* lanzalloc_realloc(struct lanzalloc* lanzalloc, void* address, lanzalloc_size_t new_size) {
	lanzalloc_size_t i, maxunit = lanzalloc->unit_max, old_size = *(unsigned int*)((char*)address - sizeof(unsigned int)), delta = new_size - old_size;
	if (address == 0) return lanzalloc_alloc(lanzalloc, new_size);
	if (new_size == 0) {
		lanzalloc_free(lanzalloc, address);
		return 0;
	}
	if (delta > 0) {
		void* tail = (void*)((byte_t*)address + old_size);
		for (i = 0; i < maxunit; i++) {
			unit_t* unit = lanzalloc->units + i;
			if (unit->size >= delta && tail == unit->address) {
				unit->size -= delta;
				unit->address = (void*)((byte_t*)unit->address + delta);
				*(lanzalloc_size_t*)((byte_t*)address - sizeof(lanzalloc_size_t)) = new_size;
				return address;
			}
		}
		return 0;
	} else if (delta < 0) {
		do {
			for (i = 0; i < maxunit; i++) {
				unit_t* unit = lanzalloc->units + i;
				// Find a unit.
				if ( UNIT_USED(unit) ) continue;
				unit->size = -delta;
				unit->address = (void*)((byte_t*)address + new_size);
				*(lanzalloc_size_t*)((byte_t*)address - sizeof(lanzalloc_size_t)) = new_size;
				return address;
			}
		} while(lanzalloc_defragment(lanzalloc));
		return 0;
	} else {
		return address;
	}
	return 0;
}

void lanzalloc_free(struct lanzalloc* lanzalloc, void* address) {
	lanzalloc_size_t i, maxunit = lanzalloc->unit_max;
	if (!address) return;
	do {
		for (i = 0; i < maxunit; i++) {
			unit_t* unit = lanzalloc->units + i;
			pool_block_t* block = (pool_block_t*)(((byte_t*)address) - sizeof(pool_block_t));
			// Find a unit.
			if ( UNIT_USED(unit) ) continue;
			UNIT_SET(unit, (void*)block->bytes, block->size_info);
			return;
		}
	} while(lanzalloc_defragment(lanzalloc));
}

lanzalloc_size_t lanzalloc_freemem(struct lanzalloc* lanzalloc) {
	lanzalloc_size_t i, result = 0, maxunit = lanzalloc->unit_max;
	for (i = 0; i < maxunit; i++) result += lanzalloc->units[i].size;
	return result;
}

lanzalloc_size_t lanzalloc_usedmem(struct lanzalloc* lanzalloc) {
	return lanzalloc->pool_size - lanzalloc_freemem(lanzalloc);
}
