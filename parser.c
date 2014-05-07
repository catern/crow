#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "types.h"
#include "gc.h"

char chbuf;			/* buffer for input chars */
gboolean char_waiting = FALSE;

char *readbuf;			// buffer for reading chars
int readbufp = 0;
size_t readlength = 0;

int
getch ()
{
    // check if there are any characters in our lookahead
    if (char_waiting) {
        // return char in our lookahead buffer
        char_waiting = FALSE;
        return (int) chbuf;
    }

    // we must get new characters

    else if (readbufp < readlength) {
        // read from the readbuf, filled from a file or (read) call
        return (int) readbuf[readbufp++];
    }

    else {
        // read from user input
        return getchar ();
    }
}

void
ungetch (int c)
{
    // store an unwanted char to be parsed again
    chbuf = c;
    char_waiting = TRUE;
}

void
gettoken (char *token)
{
    int c;
    // ignore whitespace
    while (iswspace ((c = getch ()))) {
        ;
    }

    // the first non-whitespace is the first char of our token
    token[0] = c;
    if (c == '(' || c == ')' || c == '\'' || c == '\"' || c == ';')
        // if it's a special character, return just it
    {
        token[1] = '\0';
        return;
    }

    int i = 1;
    // keep adding chars to the token until we see whitespace or a special character
    while (!iswspace (c = getch ())) {
        if (c == '(' || c == ')' || c == '\'' || c == '\"' || c == ';') {
            // special char can't be part of the token, so unget it and return
            // this char will be returned by the next call to getch,
            // so will be the next token returned by gettoken
            ungetch (c);
            token[i] = '\0';
            return;
        }
        token[i] = c;
        i++;
    }
    token[i] = '\0';
}

struct node *parse_comment ();

struct node *parse_list ();

struct node *parse_string ();

struct node *parse_quote ();

struct node *parse_token (char *token);

struct node *
parse ()
{
    /* This is the entry point for the recursive descent parser */
    char token[MAXTOKEN];
    gettoken (token);
    if (token[0] == ';') {
        return parse_comment ();
    } else if (token[0] == '(') {
        return parse_list ();
    } else if (token[0] == '\"') {
        return parse_string ();
    } else if (token[0] == '\'') {
        return parse_quote ();
    } else {
        return parse_token (token);
    }
}

struct node *
parse_comment ()
{
    int c;
    while ((c = getch ()) != '\n');
    return nil_alloc ();
    // might be able to do this instead
    /* return parse(); */
}

struct node *
parse_token (char *token)
{
    // sentinel for the end of a list, or invalid user input
    if (token[0] == ')') {
        return NULL;
    }
    // number
    else if ((isdigit (token[0]))
             || ((token[0] == '-' || token[0] == '.')
                 && isdigit (token[1]))) {
        return double_to_node (strtod (token, NULL));
    }
    // symbol
    else {
        return symbol_to_node (g_intern_string (token));
    }
}

struct node *
parse_list ()
{
    // currently lists are just arrays
    struct node **list = nlistalloc ();

    int i = 0;
    while ((list[i++] = parse ()) != NULL) {
        ;
    }

    // don't count the final NULL value in the number of elements
    return list_to_node (list, i - 1);
}

struct node *
parse_string ()
{
    struct node *result = nalloc ();
    result->type = STRING;
    result->string = stralloc ();

    int c = 0;

    // directly read from getch until we see " again
    while (((c = getch ()) != EOF) && (c != '\"')) {
        // this is an efficient operation, thanks glib
        g_string_append_c (result->string, c);
    }
    return result;
}

struct node *
parse_quote ()
{
    // we see 'foobar
    // we directly create a node that looks like (quote foobar)
    struct node *result = nalloc ();

    result->nlist = 2;
    result->type = LIST;
    result->list = nlistalloc ();

    result->list[0] = symbol_to_node (g_intern_string ("quote"));

    result->list[1] = parse ();
    return result;
}

struct node *
parse_ll ()
{
    // this parses tokens into linked lists (lists constructed from
    // pairs) instead of array list types
    // other than returning structures with linked lists instead of
    // array lists, it should be identical to parse()
    char token[MAXTOKEN];

    struct node *topnode;
    struct node *curnode;
    struct node *nextnode;

    gettoken (token);
    if (token[0] == '\"') {
        return parse_string ();
    } else if (token[0] == '(') {
        if ((nextnode = parse_ll ()) == NULL) {
            return nil_alloc ();
        } else {
            topnode = curnode = pair_to_node (nextnode, NULL);
        }

        while ((nextnode = parse_ll ()) != NULL) {
            curnode->pair->cdr = pair_to_node (nextnode, NULL);
            curnode = curnode->pair->cdr;
        }
        curnode->pair->cdr = nil_alloc ();
        return topnode;
    } else if (token[0] == '\'') {
        return pair_to_node (symbol_to_node (g_intern_string ("quote")),
                             pair_to_node (parse_ll (), nil_alloc ()));
    } else {
        return parse_token (token);
    }
}

struct node *
parse_file (char *filename)
{
    // mmaps the file and puts its location in readbuf, thanks GLib
    GError *error = NULL;
    if (!g_file_get_contents (filename, &readbuf, &readlength, &error)) {
        printf ("%s\n", error->message);
        g_error_free (error);
        return nil_alloc ();
    }

    struct node **list = nlistalloc ();

    // (begin foo bar baz) is a sequence of statements to be executed
    list[0] = symbol_to_node (g_intern_string ("begin"));
    int i = 1;

    // parses what is currently in the readbuf
    while (readbufp < readlength) {
        list[i++] = parse ();
    }

    g_free (readbuf);
    return list_to_node (list, i);
}
