#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "gc.h"

void *allocated[MAXPOINTERS];
int nextpointer = 0;

void *
malloc_mon(size_t size)
{
#ifdef MALLOC_DEBUG
    printf("malloc: nextpoiner: %d\n", nextpointer);
#endif
    if (nextpointer < MAXPOINTERS) {
        allocated[nextpointer] = malloc(size);
        return allocated[nextpointer++];
    }
    else {
        printf("malloc_mon: Golly that's a lot of pointers!\n");
        return malloc(size);
    }
}

void
compress_allocated(void)
{
    int i, j;
    for (i = 0; i < nextpointer; i++) { // search through all the possibly allocated pointers
        if (allocated[i]==NULL) { // if we find a null pointer
            for (j = i; j < nextpointer; j++) { // search the space past it
                if (allocated[j] != NULL) { // for a non-null pointer
                    allocated[i] = allocated[j]; // put the non-null pointer in the null space
                    allocated[j] = NULL; // mark the non-null's former space as free
                    break; // go back to searching for null pointers
                }
            }
        }
    }
    for (i = 0; allocated[i] != NULL; i++) // find the first null
        ;
    nextpointer = i; // change nextpointer allocation will start with the first null
}

void
garbage_collect(struct environment *env)
{
    int i, j, usedp = 0;
    void *inuse[MAXPOINTERS];
    usedp = env_pointers(inuse, env, usedp);
#ifdef MALLOC_DEBUG
    printf("pre-gc nextpointer: %d\n", nextpointer);
#endif 
    for (i = 0; i < nextpointer; i++) {
  //      printf("check\n");
        if (pointer_in_list(inuse, allocated[i], usedp))
            ;
        else {
  //          printf("FREEING GARBAGE! ");
            free(allocated[i]);
            allocated[i] = NULL;
        }
    }
    compress_allocated();
#ifdef MALLOC_DEBUG
    printf("post-gc nextpointer: %d\n", nextpointer);
#endif 
}


int
pointer_in_list(void *mlist[], void *pointer, int mn)
{
    int i;

    for (i = 0; i < mn; i++) {
        if (mlist[i] == pointer)
            return 1;
    }
    return 0;
}

/*
struct environment {
    int status;
    struct variable vars[MAXVAR];
};
*/

/*
struct variable {
    char *symbol;
    struct node *value;
};
*/

int
env_pointers(void *mlist[], struct environment *env, int mn)
{
    int i, j;

    if (pointer_in_list(mlist, (void *) env, mn))
        return mn;

    mlist[mn++] = env;

    for (i = 0; env[i].status != 0; i++) {
        for (j=0; j < (env[i].status - 1); j++) {
            mn = node_pointers(mlist, env[i].vars[j].value, mn);
            mlist[mn++] = env[i].vars[j].symbol;
        }
    }
    return mn;
}

/*
struct node {
    int type;
    char *symbol;
    double number;
    struct node *list;
    struct procedure *proc;
    struct pair *pair;
    int nlist;
    char *string;
};
*/

int
node_pointers(void *mlist[], struct node *expr, int mn)
{
    int i;

    if (pointer_in_list(mlist, (void *) expr, mn))
        return mn;

    mlist[mn++] = expr;

    switch (expr->type) {
        case LIST:
            mlist[mn++] = expr->list;
            // get the pointers from each node in the list
            for (i=0; i < expr->nlist; i++) {
                mn = node_pointers(mlist, expr->list[i], mn);
            }
            break;
        case PROC:
            mn = proc_pointers(mlist, (expr->proc), mn);
            break;
        case NUMBER:
            // ain't no pointers here!
            break;
        case SYMBOL:
            mlist[mn++] = expr->symbol;
            break;
        case STRING:
            mlist[mn++] = expr->string;
            break;
        case PAIR:
            mlist[mn++] = expr->pair;
            mn = node_pointers(mlist, expr->pair->car, mn);
            mn = node_pointers(mlist, expr->pair->cdr, mn);
            break;
        case NIL:
            break;
        default:
            mlist[mn++] = expr->symbol;
            printf("node_pointers: unknown type %d\n", expr->type);
    }

    return mn;
}

/*
struct procedure {
    char symbols[MAXTOKEN][MAXVAR];
    struct node body;
    struct environment *env;
    int nargs;
};
*/

int
proc_pointers(void *mlist[], struct procedure *proc, int mn)
{
    int i;

    if (pointer_in_list(mlist, (void *) proc, mn))
        return mn;

    mlist[mn++] = proc;

    // process the body of the proc
    mn = node_pointers(mlist, proc->body, mn);
    // process the env of the proc
    mn = env_pointers(mlist, proc->env, mn);
    // process the args of the proc 
    for (i=0; i < proc->nargs ; i++) {
        mlist[mn++] = proc->symbols[i];
    }
        

    return mn;
}
 
