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
	uint32_t index_mask;
	uint32_t tag_mask;
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
		unsigned cache_indices_size = cache_size/16;

		// count number of bits to represent the indices
		int cache_indices_nof_bits = -1;
		theCache.index_mask = 0;

		// cache is at most 16KB, therefore there will be at most 1000 rows (as each row has 16 bytes) so 15 is about 5 more than enough
		for(unsigned j=0;j<15;j++) {
			if(cache_indices_size == (1<<j)) {
				cache_indices_nof_bits = j;
				break;
			}
			theCache.index_mask += (1<<j);
		}

		theCache.index_mask = theCache.index_mask << 4;
		assert(cache_indices_nof_bits != -1);
		
		// initialise sizes
		theCache.index_size_bits = cache_indices_nof_bits;
		theCache.tag_size_bits = 32-theCache.index_size_bits-4;

		// initialise arrays
		theCache.data = (uint32_t*)malloc((1<<theCache.index_size_bits)*16); // 4 words per block
		theCache.tag = (uint32_t*)malloc((1<<theCache.index_size_bits)*4); // tag fits in one word
		theCache.valid_bit = (bool*)malloc(1<<theCache.index_size_bits);
		for(int j=0;j<(1<<theCache.index_size_bits);j++)
			theCache.valid_bit[j] = false;	

		// TODO check for unsigneds/ints problems
		for(unsigned i=0;i<theCache.tag_size_bits;i++)
			theCache.tag_mask += (((unsigned)1) << ((unsigned)(31-i))); 

		memory_stats_init(arch_state_ptr, theCache.tag_size_bits);
        /// @students: memory_stats_init(arch_state_ptr, X); <-- fill # of tag bits for cache 'X' correctly
    }
}

int get_cache_idx(int address) {
	return (address & theCache.index_mask) >> 4;
}

int get_cache_offset(int address) {
	return address&15;
}

uint32_t get_cache_tag(uint32_t address) {
	return (address&theCache.tag_mask) >> (4 + theCache.index_size_bits);
}

bool address_in_cache(int address) {
	printf("checking if in cache: address: %d, cache_idx: %d , tag: %d\n", address, get_cache_idx(address), get_cache_tag(address));
	return (theCache.valid_bit[get_cache_idx(address)]) && (theCache.tag[get_cache_idx(address)] == get_cache_tag(address));
}

// returns data on memory[address / 4]
int memory_read(int address){
	printf("memread at %d", address);
	//TODO check if this works if there is only one index -> therefore, there are no index blocks
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

		int ret;
		if(address_in_cache(address)) {
			arch_state.mem_stats.lw_cache_hits++;
			ret = (int)theCache.data[cache_idx * 4 + offset/4];
		} else {
			// copy the block, word by word
			for(int i=0;i<4;i++) 
				theCache.data[cache_idx*4 + i] = arch_state.memory[(address-offset)/4 + i]; // 
			theCache.valid_bit[cache_idx] = 1;
			theCache.tag[cache_idx] = get_cache_tag(address);
			ret = (int)theCache.data[cache_idx*4 + offset/4];
		}
		// TODO remove the line below vvv before submitting
		assert(ret == arch_state.memory[address/4]);
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.lw_cache_hits
		return ret;
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
