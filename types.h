
#define MAXTOKEN 100
#define MAXSTRING 1000
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
    char *symbol;
    double number;
    struct node **list;
    struct procedure *proc;
    struct pair *pair;
    int nlist;
    char *string;
};

struct pair {
    struct node *car;
    struct node *cdr;
};

struct variable {
    char *symbol;
    struct node *value;
};

struct environment {
    int status;
    struct variable **vars;
};

struct procedure {
    char **symbols;
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

char **
tokenlistalloc();

char *
stralloc();

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
