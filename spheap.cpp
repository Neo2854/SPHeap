#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#include "spheap.h"

#include "mem_dll.h"

#define MAXMEM 256*1024*1024
#define KVAL 28

#define RETAIN_KVAL ((size_t) 0xfffffffffffffff8)
#define SET_BLOCK_FREE ((size_t) 0xfffffffffffffffe)
#define SET_BLOCK_OCCUP ((size_t) 0x1)

#define TYPE1 ((char) 0x1)
#define TYPE2 ((char) 0x2)
#define TYPE3 ((char) 0x3)

struct buddy_info{
    mdl_node *buddy;
    size_t buddy_size;
};

typedef struct buddy_info bud_inf;

void *spheap_chunk = NULL;
mdl **asl_table = NULL;
size_t *size_table = NULL;

char spheap_verbose = 0;

size_t spheap_alloc_req_count = 0;
size_t spheap_free_req_count = 0;

size_t spheap_total_alloc_size = 0;
size_t spheap_total_allocated_with_free = 0;
size_t spheap_total_req_size = 0;

size_t spheap_split_count = 0;
size_t spheap_merge_count = 0;

clock_t spheap_alloc_clocks = 0;
clock_t spheap_free_clocks = 0;

size_t binary_search(size_t start,size_t end,size_t size){
    if(end >= start){
        size_t mid = start + (end - start)/2;

        if(size_table[mid] == size)
            return mid;

        if(size_table[mid] > size)
            return binary_search(start,mid-1,size);
        else
            return binary_search(mid+1,end,size);
    }
    
    if(start < 2*KVAL)
        return start;

    return 2*KVAL;
}

bud_inf *get_buddy(mdl_node *node,size_t node_size){
    if(node == NULL || node_size == MAXMEM)
        return NULL;

    size_t offset_addr = ((size_t) node->memptr) - ((size_t) spheap_chunk);
    size_t kval = node->metadata >> 3;
    char type = (node->metadata >> 1) & 0x3;

    size_t buddy_size;

    size_t exp = 1 << kval;

    if(type == TYPE3){
        buddy_size = (offset_addr % (4 * exp) == 0) ? (exp) : (3 * exp);
        offset_addr = (offset_addr % (4 * exp) == 0) ? (offset_addr + 3 * exp) : (offset_addr - 3 * exp);
    }
    else{
        buddy_size = (type == TYPE1) ? (exp / 2) : (2 * exp);
        offset_addr = (type == TYPE1) ? (offset_addr + exp) : (offset_addr - 2 * exp);
    }

    void *buddy_memptr = spheap_chunk + offset_addr; 

    size_t table_index = binary_search(0,2*KVAL-1,buddy_size);

    if(table_index != 2*KVAL){
        mdl_node *temp = asl_table[table_index]->head;
        size_t temp_size = size_table[table_index];

        while(temp != NULL){
            if(temp->memptr == buddy_memptr && temp_size == buddy_size)
                break;

            temp = temp->next;
        }

        if(temp != NULL){
            bud_inf *b_inf = (bud_inf*) malloc(sizeof(bud_inf));

            b_inf->buddy = temp;
            b_inf->buddy_size = temp_size;

            return b_inf;
        }
    }

    return NULL;
}

mdl_node *split_asl(mdl *dll, mdl_node *node, size_t node_size, size_t size){
    if(node == NULL || node_size == size || node_size < 3){
        spheap_total_alloc_size += node_size;
        spheap_total_allocated_with_free += node_size;

        return node;
    }

    node = remove_mdl_node(dll, node);

    mdl_node *left_split = NULL;
    mdl_node *right_split = NULL;

    size_t l_size;
    size_t r_size;

    if(node_size % 3 == 0){
        r_size = node_size / 3;
        l_size = 2 * r_size;

        if(l_size < size && r_size < size){
            spheap_total_alloc_size += node_size;
            spheap_total_allocated_with_free += node_size;

            return node;
        }

        right_split = create_mdl_node(node->memptr + l_size);
        right_split->metadata = ((node->metadata & RETAIN_KVAL) | (TYPE2 << 1));

        left_split = node;
        left_split->metadata = ((node->metadata & RETAIN_KVAL) | (TYPE1 << 1)) + (1 << 3);
    }
    else{
        r_size = node_size / 4;
        l_size = 3 * r_size;

        if(l_size < size && r_size < size){
            spheap_total_alloc_size += node_size;

            return node;
        }

        right_split = create_mdl_node(node->memptr + l_size);
        right_split->metadata = ((node->metadata & RETAIN_KVAL) | (TYPE3 << 1)) - 16;

        left_split = node;
        left_split->metadata = right_split->metadata;
    }

    if(left_split != NULL && right_split != NULL){
        size_t l_table_index = binary_search(0,2*KVAL - 1,l_size);

        if(l_table_index == 2*KVAL)
            return NULL;

        append_mdl_node(asl_table[l_table_index],left_split);

        size_t r_table_index = binary_search(0,2*KVAL - 1,r_size);

        if(r_table_index == 2*KVAL)
            return NULL;

        append_mdl_node(asl_table[r_table_index],right_split);

        if(spheap_verbose)
            spheap_split_count++;

        if(r_size >= size)
            return split_asl(asl_table[r_table_index], right_split, r_size, size);
        
        if(l_size >= size)
            return split_asl(asl_table[l_table_index], left_split, l_size, size);
    }

    return NULL;
}

void merge_asl(mdl_node *node1,size_t node1_size,mdl_node *node2,size_t node2_size){
    if(node1 == NULL || node2 == NULL)
        return;

    size_t n1_index = binary_search(0,2*KVAL-1,node1_size);
    size_t n2_index = binary_search(0,2*KVAL-1,node2_size);

    node1 = remove_mdl_node(asl_table[n1_index],node1);
    node2 = remove_mdl_node(asl_table[n2_index],node2);

    if(node1 != NULL && node2 != NULL){
        size_t p_size = node1_size + node2_size;

        mdl_node *parent_node = create_mdl_node(node1->memptr);

        if(p_size % 3 == 0){
            size_t kval = node2->metadata >> 3;

            parent_node->metadata = (kval << 3) | (TYPE3) | SET_BLOCK_FREE;
        }
        else{
            size_t kval = (node1->metadata >> 3) + 2;

            size_t p_type = ((size_t) parent_node->memptr) - ((size_t) spheap_chunk);

            p_type = (p_type >> kval) & 0x3;

            if(p_type == 0)
                parent_node->metadata = (kval << 3) | (0x1 << 1) & SET_BLOCK_FREE;
            else
                parent_node->metadata = (kval << 3) | (p_type << 1) & SET_BLOCK_FREE;
        }

        size_t table_index = binary_search(0,2*KVAL - 1,p_size);

        append_mdl_node(asl_table[table_index],parent_node);

        if(spheap_verbose)
            spheap_merge_count++;
    }
}

void *spheap_alloc(size_t size){
    clock_t start = clock();

    if(spheap_verbose)
        spheap_alloc_req_count++;

    if(size == 0)
        return NULL;

    if(spheap_chunk == NULL)
        spheap_chunk = malloc(MAXMEM);

    if(asl_table == NULL){
        asl_table = (mdl**) malloc(2 * KVAL * sizeof(mdl*));
        size_table = (size_t*) malloc(2 * KVAL * sizeof(size_t));

        asl_table[0] = create_mdl();

        size_table[0] = 1;

        for(size_t i=1;i<2*KVAL;i++){
            asl_table[i] = create_mdl();

            if(i % 2 == 0)
                size_table[i] = 3 * (1 << ((i/2) - 1));
            else
                size_table[i] = 1 << ((i+1)/2);
        }

        append_mdl_node(asl_table[2*KVAL - 1], create_mdl_node(spheap_chunk));

        asl_table[2*KVAL - 1]->head->metadata = KVAL << 3;
    }

    size_t table_index = binary_search(0,2*KVAL - 1,size);

    if(table_index == 2*KVAL){
        printf("Invalid request size %lu\n",size);

        return NULL;
    }

    mdl_node *temp = asl_table[table_index]->head;

    while (temp != NULL && (temp->metadata & 0x1) == 1)
        temp = temp->next;

    if(temp == NULL){
        for(size_t i = table_index + 1; i<2*KVAL; i++){
            temp = asl_table[i]->head;

            while (temp != NULL && (temp->metadata & 0x1) == 1)
                temp = temp->next;

            if(temp != NULL){
                temp = split_asl(asl_table[i], temp, size_table[i], size);
                break;
            }
        }
    }
    else{
        spheap_total_alloc_size += size_table[table_index];
        spheap_total_allocated_with_free += size_table[table_index];
    }

    if(temp == NULL){
        printf("External fragmentation occured with Asize = %lu, AllocArea = %.2f%%",size,spheap_total_alloc_size/(MAXMEM * 1.0));

        return temp;
    }

    temp->metadata = temp->metadata | SET_BLOCK_OCCUP;

    if(spheap_verbose)
        spheap_total_req_size += size;

    clock_t end = clock();

    spheap_alloc_clocks += (end - start);

    return temp->memptr;
}

void spheap_free(void *memptr){
    clock_t start = clock();

    if(spheap_verbose)
        spheap_free_req_count++;

    if(spheap_chunk == NULL || memptr == NULL)
        return;

    for(size_t i=0;i<2*KVAL;i++){
        mdl_node *temp = asl_table[i]->head;

        while(temp != NULL){
            if(temp->memptr == memptr)
                break;

            temp = temp->next;
        }

        if(temp != NULL){
            temp->metadata = temp->metadata & SET_BLOCK_FREE;

            bud_inf *b_inf = get_buddy(temp,size_table[i]);

            if(b_inf != NULL && (b_inf->buddy->metadata & 0x1) == 0)
                merge_asl(temp,size_table[i],b_inf->buddy,b_inf->buddy_size);

            free(b_inf);

            clock_t end = clock();

            spheap_free_clocks += (end - start);

            return;
        }
    }
}

void __spheap_reset(){
    free(spheap_chunk);

    spheap_chunk = NULL;
    asl_table = NULL;
    size_table = NULL;

    spheap_alloc_req_count = 0;
    spheap_free_req_count = 0;
    spheap_total_alloc_size = 0;
    spheap_total_allocated_with_free = 0;
    spheap_total_req_size = 0;
    spheap_split_count = 0;
    spheap_merge_count = 0;

    spheap_alloc_clocks = 0;
    spheap_free_clocks = 0;
}

void __spheap_set_verbose(){
    spheap_verbose ^= 1;
}

double __spheap_get_internal_frag(){
    double diff = spheap_total_alloc_size - spheap_total_req_size;
    double req_size = spheap_total_req_size;

    return diff/req_size;
}

void __spheap_print_performance(){
    printf("Number of allocation requests: %lu\n", spheap_alloc_req_count);
    printf("Number of free requests: %lu\n", spheap_free_req_count);
    printf("Total succesfully allocated size: %lu\n",spheap_total_alloc_size);
    printf("Total succesfully allocated request size: %lu\n",spheap_total_req_size);
    printf("Number of splits: %lu\n",spheap_split_count);
    printf("Number of buddy recombination: %lu\n",spheap_merge_count);

    double temp1 = spheap_alloc_clocks;
    double temp2 = spheap_alloc_req_count;
    printf("Average timetaken for allocation: %.16lf secs\n",(temp1/temp2)/CLOCKS_PER_SEC);

    temp1 = spheap_free_clocks;
    temp2 = spheap_free_req_count;
    printf("Average timetaken for de-allocation: %.16lf secs\n",(temp1/temp2)/CLOCKS_PER_SEC);

    printf("Internal Fragmentation: %f%%\n",__spheap_get_internal_frag()*100);
}