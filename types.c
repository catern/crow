#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include "types.h"
#include "gc.h"


// when allocating, constants are copied, strings are copied
// everything else, ownership passes to the node

struct node *
bool_to_node(gboolean bool)
{
    struct node *result = nalloc();
    result->type = NUMBER;

    if (bool) {
        result->number = 1;
    } else {
        result->number = 0;
    }

    return result;
}

struct node *
double_to_node(double num)
{
    struct node *result = nalloc();
    result->type = NUMBER;
    result->number = num;

    return result;
}

struct node *
pair_to_node(struct node *car, struct node *cdr)
{
    struct node *result = nalloc();
    result->type = PAIR;
    result->pair = pairalloc();
    result->pair->car = car;
    result->pair->cdr = cdr;
    return result;
}

struct node *
symbol_to_node(const gchar *symbol)
{
    struct node *result = nalloc();
    result->type = SYMBOL;
    result->symbol = symbol;

    return result;
}

struct node *
string_to_node(char *string)
{
    struct node *result = nalloc();
    result->type = STRING;
    result->string = stralloc(string);

    return result;
}

struct node *
procedure_to_node(const gchar **args, int n, struct node *body, struct environment **env)
{
    struct node *result = nalloc();
    result->type = PROC;
    result->proc = procalloc();

    if (args == NULL) {
        result->proc->symbols = NULL;
    } else {
        result->proc->symbols = tokenlistalloc();
        int i;
        for (i = 0; i < n; i++) {
            result->proc->symbols[i] = args[i];
        }
    }

    result->proc->nargs = n;
    result->proc->body = body;
    result->proc->env = env;

    return result;
}

struct node *
list_to_node(struct node **list, int n)
{
    struct node *result = nalloc();
    result->type = LIST;
    result->list = list;
    result->nlist = n;
    return result;
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
