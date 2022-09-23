#include<stdlib.h>

struct MEM_DLL_NODE{
    void *memptr;

    size_t metadata;

    struct MEM_DLL_NODE *prev;
    struct MEM_DLL_NODE *next;
};

typedef struct MEM_DLL_NODE mdl_node;

mdl_node *create_mdl_node(void *memptr);

struct MEM_DLL{
    struct MEM_DLL_NODE *head;
    struct MEM_DLL_NODE *tail;
};

typedef struct MEM_DLL mdl;

mdl *create_mdl();

void append_mdl_node(mdl *dll, mdl_node *new_node);
mdl_node *remove_mdl_node(mdl *dll, mdl_node *node);