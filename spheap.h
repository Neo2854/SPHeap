#define EXTERNAL_FRAGMENTATION ((void*) 1)

void *spheap_alloc(size_t size);

void spheap_free(void *memptr);

void __spheap_reset();

void __spheap_set_verbose();

void __spheap_print_performance();