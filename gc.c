#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "types.h"
#include "gc.h"

GHashTable *allocated = NULL;

void *
malloc_mon(size_t size, freefunc ff)
{
  if (allocated == NULL) allocated = g_hash_table_new(NULL, NULL);

  void *alloc = calloc(1, size);
  g_hash_table_replace(allocated, alloc, ff);

#ifdef DEBUG
  if (g_hash_table_size(allocated) > 20000) 
    {
      printf("malloc_mon: Golly that's a lot of pointers!\n");
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
    GHashTable *inuse = g_hash_table_new(NULL, NULL);
    env_pointers(inuse, env);

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

    if (g_hash_table_contains(inuse, env)) {
        return;
    }

    mark_used(inuse, env);

    for (i = 0; env[i] != NULL; i++) {
        mark_used(inuse, env[i]);
        mark_used(inuse, env[i]->vars);
        for (j = 0; env[i]->vars[j] != NULL; j++) {
            mark_used(inuse, env[i]->vars[j]);
            //mark_used(inuse, env[i]->vars[j]->symbol);
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
          //mark_used(inuse, expr->symbol);
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
    mark_used(inuse, proc->symbols);
    /* for (i=0; i < proc->nargs ; i++) { */
    /*     mark_used(inuse, proc->symbols[i]); */
    /* } */
}
