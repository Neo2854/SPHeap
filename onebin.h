void __initialize_onebin(size_t binsize);

void *onebin_alloc(size_t size);

void onebin_free(void *memptr);

void __onebin_set_verbose();

void __onebin_print_performance();