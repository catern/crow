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

struct node **
nlistalloc()
{
    struct node **nlist = (struct node **) 
      malloc_mon(sizeof(struct node *) * MAXLIST);
    return nlist;
}

char *
tokenalloc()
{
    return (char *) malloc_mon(sizeof(char[MAXTOKEN]));
}

char **
tokenlistalloc()
{
  return (char **) malloc_mon(sizeof(char[MAXTOKEN]) * MAXVAR);
}

char *
stralloc()
{
    return (char *) malloc_mon(sizeof(char[MAXSTRING]));
}

struct environment *
envalloc()
{
    return (struct environment *) malloc_mon(sizeof(struct environment));
}

struct environment **
envlistalloc()
{
    return (struct environment **) malloc_mon(sizeof(struct environment *) * MAXENV);
}

struct procedure *
procalloc()
{
    return (struct procedure *) malloc_mon(sizeof(struct procedure));
}

struct variable *
varalloc()
{
    return (struct variable *) malloc_mon(sizeof(struct variable));
}

struct variable **
varlistalloc()
{
    return (struct variable **) malloc_mon(sizeof(struct variable *) * MAXVAR);
}
