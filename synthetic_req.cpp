#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>

#include "spheap.h"

struct freeReq{
    size_t life;

    void *memptr;

    struct freeReq *next;
};

struct freeReq *req_head = NULL;
struct freeReq *req_tail = NULL;

struct freeReq *createFreeReq(size_t life, void *memptr){
    struct freeReq *node = (struct freeReq*) malloc(sizeof(struct freeReq));
    node->life = life;
    node->memptr = memptr;

    node->next = NULL;

    return node;
}

void enQueue(struct freeReq *node){
    if(req_head != NULL){
        req_tail->next = node;
        req_tail = node;

        return;
    }

    req_head = node;
    req_tail = node;
}

void deQueue(){
    struct freeReq *temp = req_head;
    
    if(temp != NULL)
        req_head = temp->next;

    free(temp);
}

double uniform_distribution(double min,double max){
    return (rand() / (RAND_MAX + 1.0) * (max - min) + min);
}

double exponential_distribution(double mean,double min,double max){
    double u = uniform_distribution(0,1);

    double res = -1 * (log(u)*mean);

    if(res < min)
        return min;
    else if (res > max)
        return max;
    
    return res;
}

int main(){
    __spheap_set_verbose();

    srand((unsigned int) time(NULL));

    int max_requests = 2000;

    //Simulation for Exponential distribution
    size_t life = 0;

    for(int i=0;i<max_requests;i++){
        life++;

        struct freeReq *temp = req_head;

        while(temp != NULL){
            if(temp->life == life){
                spheap_free(temp->memptr);

                deQueue();
            }

            temp = temp->next;
        }

        //Calculating sizes based on uniform distribution
        size_t size = (size_t) exponential_distribution(1000,100,2000);
        size_t del_time = (size_t) uniform_distribution(life + 1,life + 100);

        void *a = spheap_alloc(size);

        if(a == EXTERNAL_FRAGMENTATION)
            break;

        enQueue(createFreeReq(del_time,a));
    }
    /*
    struct freeReq *temp = req_head;

    while(temp != NULL){
        spheap_free(temp->memptr);

        deQueue();

        temp = temp->next;
    }
    */
    printf("Simulation results for Exponential distribution\n\n");
    __spheap_print_performance();

    __spheap_reset();

    printf("\n\n");

    //Simulation for Uniform distribution
    life = 0;

    for(int i=0;i<max_requests;i++){
        life++;

        struct freeReq *temp = req_head;

        while(temp != NULL){
            if(temp->life == life){
                spheap_free(temp->memptr);

                deQueue();
            }

            temp = temp->next;
        }

        //Calculating sizes based on uniform distribution
        size_t size = (size_t) uniform_distribution(100,2000);
        size_t del_time = (size_t) uniform_distribution(life + 1,life + 100);

        void *a = spheap_alloc(size);

        if(a == EXTERNAL_FRAGMENTATION)
            break;

        enQueue(createFreeReq(del_time,a));
    }

    /*
    temp = req_head;

    while(temp != NULL){
        spheap_free(temp->memptr);

        deQueue();

        temp = temp->next;
    }
    */
    printf("Simulation results for Uniform distribution\n\n");
    __spheap_print_performance();
}