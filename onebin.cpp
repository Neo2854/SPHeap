#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#include "mem_dll.h"

#define MAXMEM 256*1024*1024

void *onebin_chunk = NULL;
size_t bin_size = 4;
mdl *storage_dll = NULL;

size_t available_mem = MAXMEM;

char onebin_verbose = 0;

size_t alloc_req_count = 0;
size_t free_req_count = 0;

size_t total_alloc_size = 0;
size_t total_req_size = 0;

clock_t alloc_clocks = 0;
clock_t free_clocks = 0;

void __initialize_onebin(size_t binsize){
    if(binsize > MAXMEM){
        printf("Bin size exceeds max memory : %lu",MAXMEM);

        return;
    }

    bin_size = binsize;
}

void *onebin_alloc(size_t size){
    clock_t start = clock();

    if(onebin_verbose)
        alloc_req_count++;

    if(size == 0 || size > bin_size || available_mem <= 0)
        return NULL;

    if(onebin_chunk == NULL)
        onebin_chunk = malloc(MAXMEM);

    if(storage_dll == NULL){
        storage_dll = create_mdl();

        append_mdl_node(storage_dll,create_mdl_node(onebin_chunk));
    }

    mdl_node *node = storage_dll->head;

    while(node != NULL && node->metadata & 0x1 == 1)
        node = node->next;

    if(node != NULL){
        if(onebin_verbose){
            total_alloc_size += bin_size;
            total_req_size += size;
        }

        if(node == storage_dll->tail){
            append_mdl_node(storage_dll,create_mdl_node(node->memptr + bin_size));
        }

        node->metadata ^= 0x1;

        available_mem -= bin_size;

        clock_t end = clock();

        alloc_clocks += (end - start);

        return node->memptr;
    }

    return NULL;
}

void onebin_free(void *memptr){
    clock_t start = clock();

    if(onebin_verbose)
        free_req_count++;

    if(memptr != NULL){
        mdl_node *node = storage_dll->head;

        while(node != NULL && node->memptr != memptr)
            node = node->next;

        if(node != NULL && node->metadata & 0x1 == 1)
            node->metadata ^= 0x1;

        available_mem += bin_size;

        clock_t end = clock();

        free_clocks += (end - start);
    }
}

void __onebin_set_verbose(){
    onebin_verbose ^= 0x1;
}

void __onebin_print_performance(){
    printf("Number of allocation requests: %lu\n", alloc_req_count);
    printf("Number of free requests: %lu\n", free_req_count);
    printf("Total succesfully allocated size: %lu\n",total_alloc_size);
    printf("Total succesfully allocated request size: %lu\n",total_req_size);

    double temp1 = alloc_clocks;
    double temp2 = alloc_req_count;
    printf("Average timetaken for allocation: %.16lf secs\n",(temp1/temp2)/CLOCKS_PER_SEC);

    temp1 = free_clocks;
    temp2 = free_req_count;
    printf("Average timetaken for de-allocation: %.16lf secs\n",(temp1/temp2)/CLOCKS_PER_SEC);
}