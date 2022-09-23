#include<iostream>
#include<stdlib.h>

#include "mem_dll.h"

using namespace std;

mdl_node *create_mdl_node(void *memptr){
    mdl_node *node = (mdl_node*) malloc(sizeof(mdl_node));

    node->memptr = memptr;
    node->metadata = 0;

    node->prev = NULL;
    node->next = NULL;

    return node;
}

mdl *create_mdl(){
    mdl *dll = (mdl*) malloc(sizeof(mdl));

    dll->head = NULL;
    dll->tail = NULL;

    return dll;
}

void append_mdl_node(mdl *dll, mdl_node *new_node){
    if(dll == NULL)
        return;

    if(dll->head == NULL){
        dll->head = new_node;
        dll->tail = new_node;
    }
    else{
        dll->tail->next = new_node;
        new_node->prev = dll->tail;
        dll->tail = new_node;
    }
}

mdl_node *remove_mdl_node(mdl *dll, mdl_node *node){
    if(dll == NULL || node == NULL)
        return NULL;

    if(node->prev != NULL)
        node->prev->next = node->next;
    else
        dll->head = node->next;

    if(node->next != NULL)
        node->next->prev = node->prev;
    else
        dll->tail = node->prev;

    node->prev = NULL;
    node->next = NULL;

    return node;
}

void __debug__print_mdl(mdl *dll){
    mdl_node *node = dll->head;

    cout << "address\t\tmetadata\n\n";

    while(node != NULL){
        printf("%p\t\t%lu\n",node->memptr,node->metadata);

        node = node->next;
    }
}