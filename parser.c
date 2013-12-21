#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "types.h"

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
  struct node *result = nalloc();
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
      return symbol_to_node(token);
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
  // todo glib strings
  struct node *result = nalloc();
  result->type = STRING;
  result->string = stralloc();        

  int c, i = 0;

  while (((c = getch()) != EOF) && (c != '\"')) 
    {
      result->string[i] = c;
      i++;
    }
  result->string[i] = '\0';
  return result;
}

struct node *
parse_quote()
{
  struct node *result = nalloc();

  result->nlist = 2;
  result->type = LIST;
  result->list = nlistalloc();

  result->list[0] = nalloc();
  result->list[0]->type = SYMBOL;
  result->list[0]->symbol = tokenalloc();
  strcpy(result->list[0]->symbol, "quote");

  result->list[1] = parse();
  return result;
}

struct node *
parse_token_ll()
{
  // this parses tokens into linked lists (lists constructed from pairs) instead of array list types
  char token[MAXTOKEN];
  int c;

  struct node *topnode;
  struct node *curnode;
  struct node *nextnode;
    
  gettoken(token);
  if (token[0] == '\"') {
    int i = 0;
    curnode = nalloc();
    curnode->string = stralloc();        

    while (((c = getch()) != EOF) && (c != '\"')) {
      curnode->string[i] = c;
      i++;
    }
    curnode->type = STRING;
    curnode->string[i] = '\0';
    return curnode;
  }
  else if (token[0] == '(') {
    int i;

    topnode = curnode = nalloc();

    i = 0;
    while ((nextnode = parse_token_ll())->type != NIL) {
      curnode->type = PAIR;
      curnode->pair = pairalloc();
      curnode->pair->car = nextnode;
      curnode->pair->cdr = nalloc();
      curnode = curnode->pair->cdr;
      i++;
    }
    curnode->type = NIL;
    return topnode;
  }
  else if (token[0] == ')') {
    return nil_alloc();
  }
  else if (token[0] == '\'') {
    topnode = curnode = nalloc();

    nextnode = nalloc();
    char *quote;
    quote = tokenalloc();
    strcpy(quote, "quote");
    nextnode->type = SYMBOL;
    nextnode->symbol = quote;

    curnode->type = PAIR;
    curnode->pair = pairalloc();
    curnode->pair->car = nextnode;
    nextnode = nalloc();
    curnode->pair->cdr = nextnode;
    curnode = nextnode;

    curnode->type = PAIR;
    curnode->pair = pairalloc();
    curnode->pair->car = parse_token_ll();
    curnode->pair->cdr = nil_alloc();

    return topnode;
  }
  else if ((token[0] == '-' && isdigit(token[1])) || isdigit(token[0])) {
    /* next node will be a number */
    int n;
    n = atof(token);

    curnode = nalloc();
    curnode->type = NUMBER;
    curnode->number = n;
    return curnode;
  }
  else {
    /* next node will be a symbol */
    char *symbol;
    symbol = tokenalloc();
    strcpy(symbol, token);

    curnode = nalloc();
    curnode->type = SYMBOL;
    curnode->symbol = symbol;
    return curnode;
  }
}

struct node *
parse_file(char *filename)
{
    struct node *root = nalloc();

    // reads the entire file into readbuf
    GError *error;
    if (!g_file_get_contents(filename, &readbuf, &readlength, &error)) {
        printf("%s\n", error->message);
        g_error_free(error);
        root->type = NIL;
        return root;
    }

    root->type = LIST;
    root->list = nlistalloc();
    root->list[0] = nalloc(); 
    root->list[0]->type = SYMBOL;
    root->list[0]->symbol = tokenalloc();
    strcpy(root->list[0]->symbol,"begin");

    int i = 1;
    // parses what is currently in the queue
    while (readbufp < readlength) {
      root->list[i++] = parse();
    }
    root->nlist = i;
    g_free(readbuf);
    return root;
}
