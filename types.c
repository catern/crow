#include <stdlib.h>
#include <glib.h>
#include <string.h>
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

struct node *
nil_alloc()
{
    struct node *nil = malloc_mon(sizeof(struct node));
    nil->type = NIL;
    return nil;
}

struct node *
bool_to_node(gboolean bool)
{
  struct node *result = nalloc();
  result->type = NUMBER;

  if (bool)
    {
      result->number = 1;
    }
  else 
    {
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
symbol_to_node(char *symbol)
{
  struct node *result = nalloc();
  result->type = SYMBOL;
  result->symbol = tokenalloc();
  strcpy(result->symbol, symbol);

  return result;
}

struct node *
string_to_node(char *string)
{
  struct node *result = nalloc();
  result->type = STRING;
  result->string = stralloc();
  strcpy(result->string, string);

  return result;
}
