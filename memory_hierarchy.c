/*************************************************************************************|
|   1. YOU ARE NOT ALLOWED TO SHARE/PUBLISH YOUR CODE (e.g., post on piazza or online)|
|   2. Fill main.c and memory_hierarchy.c files                                       |
|   3. Do not use any other .c files neither alter main.h or parser.h                 |
|   4. Do not include any other library files                                         |
|*************************************************************************************/
#include "mipssim.h"

/// @students: declare cache-related structures and variables here
struct Cache {
	uint32_t *data;
	uint32_t *tag;
	bool *valid_bit;
	int tag_size_bits;
	int index_size_bits;
	int index_mask;
	int tag_mask;
};

struct Cache theCache;

void memory_state_init(struct architectural_state* arch_state_ptr) {
    arch_state_ptr->memory = (uint32_t *) malloc(sizeof(uint32_t) * MEMORY_WORD_NUM);
    memset(arch_state_ptr->memory, 0, sizeof(uint32_t) * MEMORY_WORD_NUM);
    if(cache_size == 0){
        // CACHE DISABLED
        memory_stats_init(arch_state_ptr, 0); // WARNING: we initialize for no cache 0
    }else {
        // CACHE ENABLED
		int cache_indices_size = cache_size/16;

		// count number of bits to represent the indices
		int cache_indices_nof_bits = -1;
		theCache.index_mask = 0;

		for(int i=0;i<15;i++) {
			if(cache_indices_size == (1<<i)) {
				cache_indices_nof_bits = i;
				break;
			}
			theCache.index_mask += (1<<i);
		}
		theCache.index_mask = theCache.index_mask << 4;
		assert(cache_indices_nof_bits != -1);
		
		// initialise sizes
		theCache.index_size_bits = cache_indices_nof_bits;
		theCache.tag_size_bits = 32-theCache.index_size_bits-4;

		// initialise arrays
		theCache.data = (uint32_t*)malloc((1<<theCache.index_size_bits)*16); // 4 words per block
		theCache.tag = (uint32_t*)malloc(1<<theCache.index_size_bits); // tag fits in one word
		theCache.valid_bit = (bool*)malloc(1<<theCache.index_size_bits);

		// TODO check for unsigneds/ints problems
		for(unsigned i=0;i<theCache.tag_size_bits;i++)
			theCache.tag_mask += (((unsigned)1) << ((unsigned)(31-i))); 

		memory_stats_init(arch_state_ptr, theCache.tag_size_bits);
        /// @students: memory_stats_init(arch_state_ptr, X); <-- fill # of tag bits for cache 'X' correctly
    }
}

int get_cache_idx(int address) {
	return address&theCache.index_mask >> 4;
}

int get_cache_offset(int address) {
	return address&15;
}

int get_cache_tag(int address) {
	address&theCache.tag_mask >> (4 + theCache.index_size_bits);
}

bool address_in_cache(int address) {
	return theCache.tag[get_cache_idx(address)] == get_cache_tag(address);
}

// returns data on memory[address / 4]
int memory_read(int address){
	assert(address%4==0);
    arch_state.mem_stats.lw_total++;
    check_address_is_word_aligned(address);

    if(cache_size == 0){
        // CACHE DISABLED
        return (int) arch_state.memory[address / 4];
    }else{
        // CACHE ENABLED
		int cache_idx = get_cache_idx(address);
		int offset = get_cache_offset(address);

		if(address_in_cache(address)) {
			arch_state.mem_stats.lw_cache_hits++;
			return (int)theCache.data[cache_idx * 4 + offset/4];
		} else {
			theCache.data[cache_idx*4 + offset/4] = arch_state.memory[address/4];
			return theCache.data[cache_idx*4 + offset/4];
		}
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.lw_cache_hits
    }
}

// writes data on memory[address / 4]
void memory_write(int address, int write_data){
    arch_state.mem_stats.sw_total++;
    check_address_is_word_aligned(address);

    if(cache_size == 0){
        // CACHE DISABLED
        arch_state.memory[address / 4] = (uint32_t) write_data;
    }else{
        // CACHE ENABLED
		arch_state.memory[address / 4] = (uint32_t) write_data; // as write-through
		// update cache if it is in the cache - as write-no-allocate
		if(address_in_cache(address)) {
			arch_state.mem_stats.sw_cache_hits++;
			theCache.data[get_cache_idx(address)*4 + get_cache_offset(address)/4] = (uint32_t) write_data;
		}
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.sw_cache_hits
    }
}
