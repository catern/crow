#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

struct node niller_node = { .type = NIL };


char chbuf[MAXCHARBUF]; /* buffer for input chars */
int bufp = 0; /* next free space in chbuf */

char *readbuf; // buffer for reading chars
int readbufp = 0;
int readlength = 0;

int getch()
{
    if (bufp > 0)
        return (int) chbuf[--bufp];
    else if (readbufp < readlength)
        return (int) readbuf[readbufp++];
    else
        return getchar();
}

void ungetch(int c)
{
    if (bufp >= MAXCHARBUF)
        printf("ungetch: char buffer overflow\n");
    else
        chbuf[bufp++] = c;
}

int gettoken(char *token)
{
    int c;

    while ((c = getch()) == ' ' || c == '\t' || c == '\n' )
        ;
    if (c != '(' && c != ')' && c != '\'' && c != '\"' && c != ';') {
        for (*token++ = c; !iswspace(c = getch()) && c != '(' && c != ')'; *token++ = c)
            ;
        *token = '\0';
        ungetch(c); /* the token ended but we went one char too far */
        return 0;
    }
    else
        return c;
}

struct node
parse_token()
{
    /* This is called parse_token, but it's the entirety of the parser.
     * A list is just another kind of "token", defined by parentheses and
     * some tokens within it.
     * This parser recursively parses lists when a leftparen is seen,
     * and ordinary tokens otherwise */
    char token[MAXTOKEN];
    int c;

    struct node curnode;
    
    c = gettoken(token);
    if (c == ';') {
        while ((c = getch()) != '\n')
            ;
        return niller_node;
    }
    else if (c == '(') {
        struct node *list;
        list = nlistalloc();
        int i;

        i = 0;
        while ((curnode = parse_token()).type != NIL) {
            list[i] = curnode;
            i++;
        }

        curnode.nlist = i;
        curnode.type = LIST;
        curnode.list = list;
        return curnode;
    }
    else if (c == ')') {
        return niller_node;
    }
    else if (c == '\"') {
        int i = 0;
        curnode.string = stralloc();        

        while (((c = getch()) != EOF) && (c != '\"')) {
            curnode.string[i] = c;
            i++;
        }
        curnode.type = STRING;
        curnode.string[i] = '\0';
        return curnode;
    }
     else if (c == '\'') {
        struct node *list;
        list = nlistalloc();

        char *quote;
        quote = tokenalloc();
        strcpy(quote, "quote");

        curnode.type = SYMBOL;
        curnode.symbol = quote;

        list[0] = curnode;
        list[1] = parse_token();

        curnode.nlist = 2;
        curnode.type = LIST;
        curnode.list = list;
        return curnode;
    }
    else if (((token[0] == '-' || token[0] == '.') && isdigit(token[1])) || isdigit(token[0])) {
        /* next node will be a number */
        double n;
        n = strtod(token, NULL);

        curnode.type = NUMBER;
        curnode.number = n;
        return curnode;
    }
    else {
        /* next node will be a symbol */
        char *symbol;
        symbol = tokenalloc();
        strcpy(symbol, token);

        curnode.type = SYMBOL;
        curnode.symbol = symbol;
        return curnode;
    }
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
    
    c = gettoken(token);
    if (c == '\"') {
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
    else if (c == '(') {
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
    else if (c == ')') {
        return &niller_node;
    }
    else if (c == '\'') {
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
        if (nextnode->type = NIL)
            return &niller_node;
        curnode->pair->cdr = nextnode;
        curnode = nextnode;

        curnode->type = PAIR;
        curnode->pair = pairalloc();
        curnode->pair->car = parse_token_ll();
        curnode->pair->cdr = &niller_node;

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
    FILE *readfile;
    readfile = fopen(filename, "r");
    if (readfile == NULL) {
      printf("parse_file: not a valid file: %s\n", filename);
    }
    fseek(readfile, 0, SEEK_END); // moves the position where you read from the file right to the end
    long pos = ftell(readfile); // not sure what ftell is, have to look it up
    fseek(readfile, 0, SEEK_SET); // moves the reading position to the beginning of the file again
    readbuf = malloc(pos); // ah, maybe this is getting the length of the file from the end of file position probably
    fread(readbuf, pos, 1, readfile); 
    fclose(readfile);
    readlength = pos - 1;
    // it's reading the entire file into the char *readbuf

    struct node *root = nalloc();
    root->type = LIST;
    root->list = nlistalloc();
    root->list[0] = *nalloc(); 
    root->list[0].type = SYMBOL;
    root->list[0].symbol = tokenalloc();
    strcpy(root->list[0].symbol,"begin");

    int i = 1;
    // parses what is currently in the queue
    while (readbufp < readlength) {
      root->list[i++] = parse_token();
    }
    root->nlist = i;
    return root;
}
