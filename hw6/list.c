#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "spinlock.h"

typedef struct proc_list_t {
    struct proc_list_node   *head;
    struct proc_list_node   *tail;
    volatile char           *lock;
} proc_list;

typedef struct proc_list_node {
    pid_t   pid;
    int     id;

    struct  proc_list_node *next;
} proc_node;

// Initialize list -- starts empty list with NULL head and tail pointers.
proc_list *list_init()
{
    proc_list *list = (proc_list *) mmap(NULL, sizeof(proc_list), PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    list->head = NULL;
    list->tail = NULL;
    list->lock = (char *) mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    list->lock = 0;

    return list;
}

// Push to end of list
// Creates a new node, pointing next to NULL
// Sets the current tail node's `next` pointer to point at our new node
// Sets our lists tail to be our new node.
void list_push(proc_list *list, pid_t pid, int id)
{
    proc_node *new = (proc_node *) mmap(NULL, sizeof(proc_node), PROT_READ | PROT_WRITE, 
                                       MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    new->pid = pid;
    new->id = id;
    new->next = NULL;
    
    spin_lock(list->lock);

    if(list->tail == NULL)
    {
        list->head = new;
        list->tail = new;
        
        goto exit;
    }

    list->tail->next = new;
    list->tail = new;

exit:
    spin_unlock(list->lock);
    return;
}

// pop from list
