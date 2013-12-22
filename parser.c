#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "types.h"
#include "gc.h"

char chbuf; /* buffer for input chars */
gboolean char_waiting = FALSE; 

char *readbuf; // buffer for reading chars
int readbufp = 0;
size_t readlength = 0;

int getch()
{
  if (char_waiting)
    {
      char_waiting = FALSE;
      return (int) chbuf;
    }
  else if (readbufp < readlength)
    return (int) readbuf[readbufp++];
  else
    return getchar();
}

void ungetch(int c)
{
    chbuf = c;
    char_waiting = TRUE;
}

void
gettoken(char *token)
{
  int c;
  while (iswspace((c = getch()))) 
    {
      ;
    }

  token[0] = c;
  if (c == '(' || c == ')' || c == '\'' || c == '\"' || c == ';') 
    {
      token[1] = '\0';
      return;
    }

  int i = 1;
  while (!iswspace(c = getch())) 
    {
      if (c == '(' || c == ')' || c == '\'' || c == '\"' || c == ';') 
        {
          ungetch(c);
          token[i] = '\0';
          return;
        }
      else 
        {
          token[i] = c;
        }
      i++;
    }
  token[i] = '\0';
}

struct node *
parse_comment();

struct node *
parse_list();

struct node *
parse_string();

struct node *
parse_quote();

struct node *
parse_token(char *token);

struct node *
parse()
{
  /* This is the entry point for the recursive descent parser
  */
  char token[MAXTOKEN];
  int c;

  struct node *curnode; 
    
  gettoken(token);

  if (token[0] == ';') 
    {
      return parse_comment();
    }
  else if (token[0] == '(') 
    {
      return parse_list();
    }
  else if (token[0] == '\"') 
    {
      return parse_string();
    }
  else if (token[0] == '\'') 
    {
      return parse_quote();
    }
  else 
    {
      return parse_token(token);
    }
}

struct node *
parse_comment()
{
  int c;
  while ((c = getch()) != '\n')
    ;
  return nil_alloc();
}

struct node *
parse_token(char *token)
{
  if (token[0] == ')') 
    {
      return NULL;
    }
  else if ((isdigit(token[0])) 
           || (token[0] == '-' || token[0] == '.') && isdigit(token[1])) 
    {
      /* next node will be a number */
      return double_to_node(strtod(token, NULL));
    }
  else 
    {
      /* next node will be a symbol */
      return symbol_to_node(g_intern_string(token));
    }
}

struct node *
parse_list()
{
  struct node *curnode;
  struct node *result = nalloc();
  result->type = LIST;
  result->list = nlistalloc();

  int i = 0;
  while ((result->list[i++] = parse()) != NULL)
    {
      ;
    }

  result->nlist = i-1;
  return result;
}

struct node *
parse_string()
{
  struct node *result = nalloc();
  result->type = STRING;
  result->string = stralloc();        

  int c = 0;

  while (((c = getch()) != EOF) && (c != '\"')) {
    g_string_append_c(result->string, c);
  }
  return result;
}

struct node *
parse_quote()
{
  struct node *result = nalloc();

  result->nlist = 2;
  result->type = LIST;
  result->list = nlistalloc();

  result->list[0] = symbol_to_node(g_intern_string("quote"));

  result->list[1] = parse();
  return result;
}

struct node *
parse_ll()
{
  // this parses tokens into linked lists (lists constructed from pairs) instead of array list types
  char token[MAXTOKEN];
  int c;

  struct node *topnode;
  struct node *curnode;
  struct node *nextnode;
    
  gettoken(token);
  if (token[0] == '\"')
    {
      return parse_string();
    }
  else if (token[0] == '(') 
    {
      if ((nextnode = parse_ll()) == NULL)
        {
          return nil_alloc();
        }
      else
        {
          topnode = curnode = pair_to_node(nextnode, NULL);
        }

      while ((nextnode = parse_ll()) != NULL) 
        {
          curnode->pair->cdr = pair_to_node(nextnode, NULL);
          curnode = curnode->pair->cdr;
        }
      curnode->pair->cdr = nil_alloc();
      return topnode;
    }
  else if (token[0] == '\'')
    {
      return pair_to_node(symbol_to_node(g_intern_string("quote")),
                          pair_to_node(parse_ll(),
                                       nil_alloc()));
    }
  else 
    {
      return parse_token(token);
    }
}

struct node *
parse_file(char *filename)
{
    // reads the entire file into readbuf
    GError *error;
    if (!g_file_get_contents(filename, &readbuf, &readlength, &error)) {
        printf("%s\n", error->message);
        g_error_free(error);
        return nil_alloc();
    }

    struct node **list = nlistalloc();

    list[0] = symbol_to_node(g_intern_string("begin"));
    int i = 1;

    // parses what is currently in the queue
    while (readbufp < readlength) {
      list[i++] = parse();
    }

    g_free(readbuf);
    return list_to_node(list, i);
}
