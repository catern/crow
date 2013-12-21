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

void
mark_used(GHashTable *inuse, void *pointer)
{
    g_hash_table_add(inuse, pointer);
}

void
node_pointers(GHashTable *inuse, struct node *expr);

void
env_pointers(GHashTable *inuse,  struct environment **env);

void
proc_pointers(GHashTable *inuse,  struct procedure *proc);

void
garbage_collect(struct environment **env)
{
    int i = 0;
    GHashTable *inuse = g_hash_table_new(NULL, NULL);

    env_pointers(inuse, env);
#ifdef DEBUG
    printf("pre-gc nextpointer: %d\n", nextpointer);
#endif 
    for (i = 0; i < nextpointer; i++) {
        if (!g_hash_table_contains(inuse, allocated[i])) {
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
env_pointers(GHashTable *inuse,  struct environment **env)
{
    int i, j;

    if (g_hash_table_contains(inuse, env)) {
        return;
    }

    mark_used(inuse, env);

    for (i = 0; env[i] != NULL; i++) {
        mark_used(inuse, env[i]);
        mark_used(inuse, env[i]->vars);
        for (j = 0; env[i]->vars[j] != NULL; j++) {
            mark_used(inuse, env[i]->vars[j]);
            mark_used(inuse, env[i]->vars[j]->symbol);
            node_pointers(inuse, env[i]->vars[j]->value);
        }
    }
    return;
}

void
node_pointers(GHashTable *inuse, struct node *expr)
{
    int i;

    if (g_hash_table_contains(inuse, expr)) {
        return;
    }

    mark_used(inuse, expr);

    switch (expr->type) {
        case LIST:
            mark_used(inuse, expr->list);
            // get the pointers from each node in the list
            for (i=0; i < expr->nlist; i++) {
                node_pointers(inuse, expr->list[i]);
            }
            break;
        case PROC:
            proc_pointers(inuse, expr->proc);
            break;
        case NUMBER:
            // ain't no pointers here!
            break;
        case SYMBOL:
            mark_used(inuse, expr->symbol);
            break;
        case STRING:
            mark_used(inuse, expr->string);
            break;
        case PAIR:
            mark_used(inuse, expr->pair);
            node_pointers(inuse, expr->pair->car);
            node_pointers(inuse, expr->pair->cdr);
            break;
        case NIL:
            break;
        default:
            mark_used(inuse, expr->symbol);
            printf("node_pointers: unknown type %d\n", expr->type);
    }
}

void
proc_pointers(GHashTable *inuse,  struct procedure *proc)
{
    int i;

    if (g_hash_table_contains(inuse, proc)) {
        return;
    }

    mark_used(inuse, proc);

    // process the body of the proc
    node_pointers(inuse, proc->body);
    // process the env of the proc
    env_pointers(inuse, proc->env);
    // process the args of the proc 
    for (i=0; i < proc->nargs ; i++) {
        mark_used(inuse, proc->symbols[i]);
    }
}
