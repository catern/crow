#include <stdlib.h>
#include "types.h"
#include "gc.h"

struct pair *
pairalloc()
{
    return (struct pair *) malloc_mon(sizeof(struct pair));
}

struct node *
nalloc()
{
    return (struct node *) malloc_mon(sizeof(struct node));
}

struct node *
nlistalloc()
{
    return (struct node *) malloc_mon(sizeof(struct node[MAXLIST]));
}

char *
tokenalloc()
{
    return (char *) malloc_mon(sizeof(char[MAXTOKEN]));
}

char *
stralloc()
{
    return (char *) malloc_mon(sizeof(char[MAXSTRING]));
}

struct environment *
envlistalloc()
{
    return (struct environment *) malloc_mon(sizeof(struct environment[MAXENV]));
}

struct procedure *
procalloc()
{
    return (struct procedure *) malloc_mon(sizeof(struct procedure));
}
