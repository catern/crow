#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "types.h"
#include "gc.h"

// hash table of all allocated memory
// key: pointer to memory
// value: function pointer of type to free memory
GHashTable *allocated = NULL;

void *
malloc_mon(size_t size, freefunc ff)
{
  if (allocated == NULL) 
    {
      allocated = g_hash_table_new(NULL, NULL);
    }

  void *alloc = calloc(1, size);
  g_hash_table_replace(allocated, alloc, ff);

#ifdef DEBUG
  if (g_hash_table_size(allocated) > 20000) 
    {
      printf("malloc_mon: %d pointers in memory table\n", g_hash_table_size(allocated));
    }
#endif

  return alloc;
}

struct pair *
pairalloc()
{
  return (struct pair *) malloc_mon(sizeof(struct pair), &free);
}

struct node *
nalloc()
{
  return (struct node *) malloc_mon(sizeof(struct node), &free);
}

struct node **
nlistalloc()
{
  // TODO use glib lists
  struct node **nlist = (struct node **) 
    malloc_mon(sizeof(struct node *) * MAXLIST, &free);
  return nlist;
}

const gchar **
tokenlistalloc()
{
  return (const gchar **) malloc_mon(sizeof(const gchar *) * MAXVAR, &free);
}

struct environment *
envalloc()
{
  return (struct environment *) malloc_mon(sizeof(struct environment), &free);
}

struct environment **
envlistalloc()
{
  // TODO use glib lists
  return (struct environment **) malloc_mon(sizeof(struct environment *) * MAXENV, &free);
}

struct procedure *
procalloc()
{
  return (struct procedure *) malloc_mon(sizeof(struct procedure), &free);
}

struct variable *
varalloc()
{
  return (struct variable *) malloc_mon(sizeof(struct variable), &free);
}

struct variable **
varlistalloc()
{
  return (struct variable **) malloc_mon(sizeof(struct variable *) * MAXVAR, &free);
}

struct node *
nil_alloc()
{
  // TODO switch to returning a pointer to a single nil node, 
  // and make the free function a dummy
  struct node *nil = malloc_mon(sizeof(struct node), &free);
  nil->type = NIL;
  return nil;
}

GString *
stralloc(char *init)
{
  GString *string = g_string_new(init);
  g_hash_table_replace(allocated, string, &g_string_free);
  return string;
}

void
mark_used(GHashTable *inuse, void *pointer)
{
  g_hash_table_add(inuse, pointer);
}

void
node_pointers(GHashTable *inuse, struct node *expr);

void
env_pointers(GHashTable *inuse, struct environment **env);

void
proc_pointers(GHashTable *inuse, struct procedure *proc);

gboolean
free_if_not(gpointer key, gpointer ff, gpointer inuse) 
{
  if (!g_hash_table_contains(inuse, key)) 
    {
      ((freefunc) ff)(key);
      return TRUE;
    }
  return FALSE;
}

void
garbage_collect(struct environment **env)
{
#ifdef DEBUG
    printf("pre-gc # of pointers: %d\n", g_hash_table_size(allocated));
#endif 
    // make a table (more like a set) for all in use pointers
    GHashTable *inuse = g_hash_table_new(NULL, NULL);

    // run through the env and add all pointers in it to the inuse table
    env_pointers(inuse, env);

    // ahh it's almost like english!
    // anticipates free_if_not returning a boolean and removes the
    // pointer from the allocated table if it returns TRUE
    g_hash_table_foreach_remove(allocated, free_if_not, inuse);

    g_hash_table_destroy(inuse);

#ifdef DEBUG
    printf("post-gc # of pointers: %d\n", g_hash_table_size(allocated));
#endif 
}

void
env_pointers(GHashTable *inuse,  struct environment **env)
{
    int i, j;

    if (g_hash_table_contains(inuse, env)) 
      {
        return;
      }

    // mark the env itself as in-use
    mark_used(inuse, env);

    // mark everything the node references as in-use
    for (i = 0; env[i] != NULL; i++) {
        mark_used(inuse, env[i]);
        mark_used(inuse, env[i]->vars);
        for (j = 0; env[i]->vars[j] != NULL; j++) {
            mark_used(inuse, env[i]->vars[j]);
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

    // mark the node itself as in-use
    mark_used(inuse, expr);

    // mark everything the node references as in-use
    switch (expr->type) {
        case LIST:
          {
            mark_used(inuse, expr->list);
            // get the pointers from each node in the list
            for (i=0; i < expr->nlist; i++) {
                node_pointers(inuse, expr->list[i]);
            }
            break;
          }
        case PROC:
          {
            proc_pointers(inuse, expr->proc);
            break;
          }
        case STRING:
          {
            mark_used(inuse, expr->string);
            break;
          }
        case PAIR:
          {
            mark_used(inuse, expr->pair);
            node_pointers(inuse, expr->pair->car);
            node_pointers(inuse, expr->pair->cdr);
            break;
          }
        case SYMBOL:
        case NUMBER:
        case NIL:
          {
            // these types don't reference anything
            // symbol strings are interned, not allocated, so they
            // won't be gc'd
            break;
          }
        default:
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

    // mark the procedure itself as in-use
    mark_used(inuse, proc);

    // mark everything the procedure references as in-use

    // process the body of the proc
    node_pointers(inuse, proc->body);
    // process the env of the proc
    env_pointers(inuse, proc->env);
    // process the args of the proc 
    mark_used(inuse, proc->symbols);
}
