#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <glib.h>
#include "types.h"
#include "gc.h"

void *allocated[MAXPOINTERS];
int nextpointer = 0;

void *
malloc_mon(size_t size)
{
#ifdef DEBUG_GC
    printf("malloc: nextpoiner: %d\n", nextpointer);
#endif
    if (nextpointer < MAXPOINTERS) {
        allocated[nextpointer] = calloc(1, size);
        return allocated[nextpointer++];
    }
    else {
        printf("malloc_mon: Golly that's a lot of pointers!\n");
        return calloc(1, size);
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

int
ptr_compare(uintptr_t a, uintptr_t b)
{
  return (a > b) - (a < b);
}

void
mark_used(GTree *inusetree, void *pointer)
{
    g_tree_insert(inusetree, pointer, (gpointer *) 1);
}

void
node_pointers(GTree *inusetree, struct node *expr);

void
env_pointers(GTree *inusetree,  struct environment **env);

void
proc_pointers(GTree *inusetree,  struct procedure *proc);

void
garbage_collect(struct environment **env)
{
    int i, j, usedp = 0;
    void *inuse[MAXPOINTERS];
    GTree *inusetree = g_tree_new((GCompareFunc) ptr_compare);

    env_pointers(inusetree, env);
#ifdef DEBUG
    printf("pre-gc nextpointer: %d\n", nextpointer);
#endif 
    for (i = 0; i < nextpointer; i++) {
        if (g_tree_lookup(inusetree, allocated[i]) == NULL) {
            free(allocated[i]);
            allocated[i] = NULL;
        }
    }
    compress_allocated();
#ifdef DEBUG
    printf("post-gc nextpointer: %d\n", nextpointer);
#endif 
}

void
env_pointers(GTree *inusetree,  struct environment **env)
{
    int i, j;

    if (g_tree_lookup(inusetree, env) != NULL) {
        return;
    }

    mark_used(inusetree, env);

    for (i = 0; env[i] != NULL; i++) {
        mark_used(inusetree, env[i]);
        mark_used(inusetree, env[i]->vars);
        for (j = 0; env[i]->vars[j] != NULL; j++) {
            mark_used(inusetree, env[i]->vars[j]);
            mark_used(inusetree, env[i]->vars[j]->symbol);
            node_pointers(inusetree, env[i]->vars[j]->value);
        }
    }
    return;
}

void
node_pointers(GTree *inusetree, struct node *expr)
{
    int i;

    if (g_tree_lookup(inusetree, expr) != NULL) {
        return;
    }

    mark_used(inusetree, expr);

    switch (expr->type) {
        case LIST:
            mark_used(inusetree, expr->list);
            // get the pointers from each node in the list
            for (i=0; i < expr->nlist; i++) {
                node_pointers(inusetree, expr->list[i]);
            }
            break;
        case PROC:
            proc_pointers(inusetree, expr->proc);
            break;
        case NUMBER:
            // ain't no pointers here!
            break;
        case SYMBOL:
            mark_used(inusetree, expr->symbol);
            break;
        case STRING:
            mark_used(inusetree, expr->string);
            break;
        case PAIR:
            mark_used(inusetree, expr->pair);
            node_pointers(inusetree, expr->pair->car);
            node_pointers(inusetree, expr->pair->cdr);
            break;
        case NIL:
            break;
        default:
            mark_used(inusetree, expr->symbol);
            printf("node_pointers: unknown type %d\n", expr->type);
    }
}

void
proc_pointers(GTree *inusetree,  struct procedure *proc)
{
    int i;

    if (g_tree_lookup(inusetree, proc) != NULL) {
        return;
    }

    mark_used(inusetree, proc);

    // process the body of the proc
    node_pointers(inusetree, proc->body);
    // process the env of the proc
    env_pointers(inusetree, proc->env);
    // process the args of the proc 
    for (i=0; i < proc->nargs ; i++) {
        mark_used(inusetree, proc->symbols[i]);
    }
}
