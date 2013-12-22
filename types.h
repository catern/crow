#include <glib.h>

#define MAXTOKEN 100
#define MAXCHARBUF 100

#define MAXVAR 200
#define MAXENV 100

#define MAXLIST 50

enum 
// data types
{ NIL = 1, LIST, SYMBOL, NUMBER, STRING, PROC, PAIR, 
//special forms
IF, LAMBDA, DEFINE, DELAY, QUOTE, COND, LET, SETCAR, SETCDR, LOAD};

//remember to edit node_copy when editing this
struct node {
    int type;
    const gchar *symbol;
    double number;
    struct node **list;
    struct procedure *proc;
    struct pair *pair;
    int nlist;
    GString *string;
};

struct pair {
    struct node *car;
    struct node *cdr;
};

struct variable {
    const gchar *symbol;
    struct node *value;
};

struct environment {
    int status;
    struct variable **vars;
};

struct procedure {
    const gchar **symbols;
    struct node *body;
    struct environment **env;
    int nargs;
};

struct pair *
pairalloc();

struct node *
nalloc();

struct node **
nlistalloc();

char *
tokenalloc();

const gchar **
tokenlistalloc();

struct environment *
envalloc();

struct environment **
envlistalloc();

struct procedure *
procalloc();

struct variable *
varalloc();

struct variable **
varlistalloc();

struct node *
nil_alloc();

struct node *
bool_to_node(gboolean bool);

struct node *
double_to_node(double num);

struct node *
pair_to_node(struct node *car, struct node *cdr);

struct node *
symbol_to_node(const gchar *symbol);

struct node *
string_to_node(char *string);

struct node *
procedure_to_node(const gchar **args, int n, struct node *body, struct environment **env);

struct node *
list_to_node(struct node **list, int n);
