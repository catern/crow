/* insert GPLv3+ here */
/* TODO: 
   switch to union types
   switch to passing pointers around
*/
#include <stdio.h>
#include <stdlib.h>

#define MAXTOKEN 100
#define MAXSTRING 1000
#define MAXCHARBUF 100
#define MAXPOINTERS 20000

#define TRUE 1
#define FALSE 0

void *allocated[MAXPOINTERS];
int nextpointer = 0;

void *
malloc_mon(size_t);

void *
malloc_mon(size_t size)
{
#ifdef MALLOC_DEBUG
    printf("malloc: nextpoiner: %d\n", nextpointer);
#endif
    if (nextpointer < MAXPOINTERS) {
        allocated[nextpointer] = malloc(size);
        return allocated[nextpointer++];
    }
    else {
        printf("malloc_mon: Golly that's a lot of pointers!\n");
        return malloc(size);
    }
}


char *strcpy(char *dest, const char *src);
int getch(void);
void ungetch(int);

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

#define MAXLIST 20

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
    struct node *list;
    struct procedure *proc;
    struct pair *pair;
    int nlist;
    char *string;
};

struct node nil_node = { .type = NIL };

struct node true_node = { .type = NUMBER, .number = 1 };
struct node false_node = { .type = NUMBER, .number = 0 };

struct pair {
    struct node *car;
    struct node *cdr;
};

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
        return nil_node;
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
        return nil_node;
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
        return &nil_node;
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
            return &nil_node;
        curnode->pair->cdr = nextnode;
        curnode = nextnode;

        curnode->type = PAIR;
        curnode->pair = pairalloc();
        curnode->pair->car = parse_token_ll();
        curnode->pair->cdr = &nil_node;

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


void
print_list(struct node expr)
{
    int i;
    printf("(");
    for (i = 0; i < expr.nlist; i++) {
        switch (expr.list[i].type) {
            case LIST:
                print_list(expr.list[i]);
                break;
            case SYMBOL:
                printf("%s ", expr.list[i].symbol);
                break;
            case NUMBER:
                printf("%f ", expr.list[i].number);
                break;
        }
    }
    if (i != 0)
        printf("\b) ");
    else
        printf(") ");
}

/* READ STEP, COMPLETE! */

/* EVAL STEP, BEGIN! */

#define MAXVAR 200
#define MAXENV 100

struct variable {
    char *symbol;
    struct node *value;
};

struct environment {
    int status;
    struct variable vars[MAXVAR];
};

struct procedure {
    char symbols[MAXTOKEN][MAXVAR];
    struct node body;
    struct environment *env;
    int nargs;
};

struct node
eval_setcar(struct node, struct environment *);

struct node
eval_setcdr(struct node, struct environment *);

void
minimal_print_node(struct node);

struct node
apply(struct node, struct node *, int);

struct node
apply_prim(struct node, struct node *, int);

struct node
create_procedure(char **, int, struct node, struct environment *);

struct node
eval_if(struct node, struct environment *);

struct node
eval_cond(struct node, struct environment *);

struct node
eval_let(struct node, struct environment *);

struct node
read_list(void);

struct node
eval_define(struct node, struct environment *);

struct node
eval_lambda(struct node, struct environment *);

struct node
eval_delay(struct node, struct environment *);

struct node
eval_force(struct node, struct environment *);

struct node
eval_quote(struct node, struct environment *);

struct node
eval_application(struct node, struct environment *);

struct node
eval_load(struct node, struct environment *);

struct variable *
lookup(struct environment *, struct node);

struct environment *
copy_environment_list(struct environment *);

struct variable undefinedvar;

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

struct node *
node_copy(struct node);

struct node *
node_copy(struct node oldnode)
{
    struct node *newnode;
    char *string;
    int i;
    newnode = nalloc();

    switch (oldnode.type) {
        case LIST:
            (*newnode).list = nlistalloc();
            for (i=0; i < oldnode.nlist; i++)
                (*newnode).list[i] = *node_copy(oldnode.list[i]);
            (*newnode).nlist = oldnode.nlist;
            break;
        case NUMBER:
            (*newnode).number = oldnode.number;
            break;
        case STRING:
            string = stralloc();
            strcpy(string, oldnode.string);
            newnode->string = string;
            break;
        case SYMBOL:
            (*newnode).symbol = oldnode.symbol;
            break;
        case PROC:
            (*newnode).proc = procalloc();
            // env
            (*(*newnode).proc).env = copy_environment_list((*oldnode.proc).env);
            // args
            for (i = 0; i < (*oldnode.proc).nargs; i++)
                strcpy((*(*newnode).proc).symbols[i],(*oldnode.proc).symbols[i]);
            (*(*newnode).proc).nargs = (*oldnode.proc).nargs;
            // body
            (*(*newnode).proc).body = *node_copy((*oldnode.proc).body);
            break;
        case PAIR:
            (*newnode).pair = pairalloc();
            (*(*newnode).pair).car = node_copy(*(*oldnode.pair).car);
            (*(*newnode).pair).cdr = node_copy(*(*oldnode.pair).cdr);
            break;
        case NIL:
            break;
        default:
            (*newnode).symbol = oldnode.symbol;
            printf("remember to update node_copy for type %d\n", oldnode.type);
    }
    (*newnode).type = oldnode.type;
    return newnode;
}


// environments

struct environment *
extend_envlist(struct environment *envlist, struct variable *varlist, int n)
{
    int i;
    struct environment newenv;

    // setup the new env
    for (i=0; i < n; i++)
        newenv.vars[i] = varlist[i];
    newenv.status = i+1;

    //find the end of the envlist
    for (i=0; envlist[i].status != 0; i++)
        ;
    // set the end of the envlist to newenv
    envlist[i] = newenv;
    // designate a new end
    envlist[i+1].status = 0;
    return envlist;
}

struct environment *
copy_environment_list(struct environment *oldenv)
{
    int i, j;
    struct environment *newenv;
    newenv = envlistalloc();
    for (i = 0; oldenv[i].status != 0; i++) {
        for (j=0; j < (oldenv[i].status - 1); j++) { 
            newenv[i].vars[j].symbol = oldenv[i].vars[j].symbol;
            newenv[i].vars[j].value = oldenv[i].vars[j].value;
        }
        newenv[i].status = oldenv[i].status;
    }
    newenv[i].status = 0;
    return newenv;
}

struct environment *
bind_in_current_env(struct environment *envlist, struct variable var)
{
    int i, j;
    struct variable *look;
    struct node temp;

    temp.symbol = var.symbol;

    // first look it up 
    look = lookup(envlist, temp);
    if (look != &undefinedvar) { 
        // if it already exists, just change the value pointer to the new one
        (*look).value = var.value;
        return envlist;
    }

    for (i=0; envlist[i].status != 0; i++)
        ;
    i--; // bind in the current environment, not a new one
    for (j=0; j < (envlist[i].status - 1); j++)
        ;
    envlist[i].status += 1;
    envlist[i].vars[j] = var;
    return envlist;
}

struct variable *
lookup(struct environment *envlist, struct node expr)
{   
    int i, j;
    struct variable *result;

    // find the end of the envlist
    for (i=0; envlist[i].status != 0; i++)
        ;
    //run through it backwards (skipping the terminal env)
    for (i = i-1; i >= 0; i--) {
        // scan through varlist until reaching end 
        for (j=0; j < (envlist[i].status - 1); j++) { 
            // if the symbols match, you've found the variable
            if (strcmp(envlist[i].vars[j].symbol, expr.symbol) == 0) {
                result = &(envlist[i].vars[j]);
                return result;
            }
        }
    }
    printf("lookup: looked up a nonexistent variable: %s\n", expr.symbol);
    return &undefinedvar;
}

struct node
lookup_value(struct environment *envlist, struct node expr)
{
    struct variable *var;
    var = lookup(envlist, expr);
    if (var != &undefinedvar)
        return *node_copy(*(*var).value);
    else
        return nil_node;
        printf("lookup_value: looked up a nonexistent variable: %s\n", expr.symbol);
}


// evaluation

int
special_form(struct node expr)
{
    if (expr.type == SYMBOL) {
        if (!strcmp("if", expr.symbol)) 
            return IF;
        else if (!strcmp("define", expr.symbol)) 
            return DEFINE;
        else if (!strcmp("lambda", expr.symbol)) 
            return LAMBDA;
        else if (!strcmp("delay", expr.symbol)) 
            return DELAY;
        else if (!strcmp("quote", expr.symbol)) 
            return QUOTE;
        else if (!strcmp("cond", expr.symbol)) 
            return COND;
        else if (!strcmp("let", expr.symbol)) 
            return LET;
        else if (!strcmp("set-car!", expr.symbol)) 
            return SETCAR;
        else if (!strcmp("set-cdr!", expr.symbol)) 
            return SETCDR;
        else if (!strcmp("load", expr.symbol)) 
            return LOAD;
        else
            return 0;
    }
    else
        return 0;
}


struct node
eval(struct node expr, struct environment *env)
{
    switch (expr.type) {
        case NUMBER: // these fundamental data types are self-evaluating
        case PAIR:
        case STRING:
        case NIL:
            return expr;
            break;
        case DELAY: // this is specially not evaluated until forced
            return expr;
            break;
        case LIST:
            switch (special_form(expr.list[0])) { 
                // special syntactic forms need special handling
                // (i.e., forms where you can't simply eval all the arguments)
                case IF:
                    return eval_if(expr, env);
                    break;
                case COND:
                    return eval_cond(expr, env);
                    break;
                case DEFINE:
                    return eval_define(expr, env);
                    break;
                case LAMBDA:
                    return eval_lambda(expr, env);
                    break;
                case DELAY:
                    return eval_delay(expr, env);
                    break;
                case QUOTE:
                    return eval_quote(expr, env);
                    break;
                case LET:
                    return eval_let(expr, env);
                    break;
                case SETCAR:
                    return eval_setcar(expr, env);
                    break;
                case SETCDR:
                    return eval_setcdr(expr, env);
                    break;
                case LOAD:
                    return eval_load(expr, env);
                    break;
                default:
                    return eval_application(expr, env);
            }
            break;
        case SYMBOL:
            if (primitive_proc(expr))
                return expr;
            else {
                return lookup_value(env, expr);
            }
            break;
        default:
            printf("eval: Unknown expression type %d;\n", expr.type);
    }
}

struct node
eval_setcar(struct node expr, struct environment *env)
{
    struct variable *var;
    var = lookup(env,expr.list[1]);
    var->value->pair->car = node_copy(eval(expr.list[2], env));
    return nil_node;
}

struct node
eval_setcdr(struct node expr, struct environment *env)
{
    struct variable *var;
    var = lookup(env,expr.list[1]);
    var->value->pair->cdr = node_copy(eval(expr.list[2], env));
    return nil_node;
}

struct node
eval_let(struct node expr, struct environment *env)
{
    struct environment *newenv;
    struct variable varlist[expr.list[1].nlist];
    struct node body;
    newenv = copy_environment_list(env);
    int i;

    for (i = 0; i < expr.list[1].nlist; i++) {
        varlist[i].symbol = expr.list[1].list[i].list[0].symbol;
        varlist[i].value = node_copy(eval(expr.list[1].list[i].list[1], env));
    }

    struct node *allocbody;
    extend_envlist(newenv, varlist, i);
    expr.list[1].type = SYMBOL;
    expr.list[1].symbol = tokenalloc();
    strcpy(expr.list[1].symbol,"begin");
    body.type = LIST;
    body.list = (expr.list+1);
    body.nlist = expr.nlist-1;
    allocbody = node_copy(body);
    // body.list will get gc'd if we don't copy it, since it isn't 
    // actually allocated, it's an allocated_pointer+1

    return eval(*allocbody, newenv);
}

struct node
eval_cond(struct node expr, struct environment *env)
{
    int i;
    for (i = 1; i < expr.nlist; i++) {
        if ((expr.list[i].list[0].type == SYMBOL && 
                    !strcmp(expr.list[i].list[0].symbol,"else")) ||
            istrue(expr.list[i].list[0], env)) {
            return eval(expr.list[i].list[1], env);
        }
    }
}

struct node
eval_quote(struct node expr, struct environment *env)
{
    return expr.list[1];
}

struct node
eval_delay(struct node expr, struct environment *env)
{
    char **empty;
    return create_procedure(empty, 0, expr.list[1], env);
}

struct node
eval_lambda(struct node expr, struct environment *env)
{
    char *arglist[expr.list[1].nlist];
    int i, dot;
    struct node body;
    struct node proc;

    // copies the tokens from the list in the second position to another array
    for (i = 0; i < expr.list[1].nlist; i++) {
        arglist[i] = (char *) malloc_mon(sizeof(char)*MAXTOKEN);
        strcpy(arglist[i], expr.list[1].list[i].symbol);
    }

    body = expr.list[2];

    proc = create_procedure(arglist, i, body, env);
    return proc;
}

struct node
eval_define(struct node expr, struct environment *env)
{
    struct variable var;
    struct node *varvalue;
    varvalue = nalloc();

    if (expr.list[1].type == LIST) {
        var.symbol = expr.list[1].list[0].symbol;
        var.value = varvalue; 
        bind_in_current_env(env, var);

        struct node body;
        char *arglist[MAXVAR];
        int i;
        for (i = 1; i < expr.list[1].nlist; i++) {
            arglist[(i-1)] = (char *) malloc_mon(sizeof(char)*MAXTOKEN);
            strcpy(arglist[(i-1)], expr.list[1].list[i].symbol);
        }
        struct node *allocbody;
        expr.list[1].type = SYMBOL;
        expr.list[1].symbol = tokenalloc();
        strcpy(expr.list[1].symbol,"begin");
        body.type = LIST;
        body.nlist = expr.nlist-1;
        body.list = expr.list+1;

        allocbody = node_copy(body); 
        // body.list will get gc'd if we don't copy it, since it isn't 
        // actually allocated, it's an allocated_pointer+1

        (*varvalue) = create_procedure(arglist, (i - 1), *allocbody, env);
    }
    else {
        var.symbol = expr.list[1].symbol;
        var.value = varvalue; 
        bind_in_current_env(env, var);

        (*varvalue) = eval(expr.list[2], env);
    }
    return (*varvalue);
}

struct node
eval_load(struct node expr, struct environment *env)
{
    if (expr.list[1].type == STRING) {
        char* filename;
        filename = expr.list[1].string;
        // open a file and read it
        FILE *readfile;
        readfile = fopen(filename, "r");
        if (readfile == NULL) {
            printf("load: not a valid file: %s\n", filename);
            return nil_node;
        }
        fseek(readfile, 0, SEEK_END);
        long pos = ftell(readfile);
        fseek(readfile, 0, SEEK_SET);
        readbuf = malloc(pos);
        fread(readbuf, pos, 1, readfile);
        fclose(readfile);
        readlength = pos - 1;

        // parse what is currently awaitin' to be parsed
        while (readbufp < readlength) {
            eval(parse_token(), env);
        }
    }
    return nil_node;
}

int
istrue(struct node expr, struct environment *env)
{
    expr = eval(expr, env);

    if (expr.type == NUMBER && expr.number == 0)
        return 0;
    else
        return 1;
}

int
node_equal(struct node n1, struct node n2)
{
    if (n1.type == SYMBOL && n2.type == SYMBOL) {
        if (!strcmp(n1.symbol,n2.symbol))
            return 1;
    }
    else if (n1.type == NUMBER && n2.type == NUMBER) {
        if (n1.number == n2.number)
            return 1;
    }
    else if (n1.type == NIL && n2.type == NIL) 
            return 1;
    return 0;
}

struct node
eval_if(struct node expr, struct environment *env)
{
    if (istrue(expr.list[1], env))
        return eval(expr.list[2], env);
    else
        return eval(expr.list[3], env);
}

struct node
eval_application(struct node expr, struct environment *env)
{
    struct node proc;
    int i;

    for (i = 0; i < expr.nlist; i++)
        expr.list[i] = eval(expr.list[i], env);

    return apply(expr.list[0], (expr.list+1), (i-1));
}

struct node *
list_to_ll(struct node oldnode)
{
    // this can definitely be made more elegant
    struct node *topnode, *curnode, *nextnode;
    int i;

    topnode = nextnode = nalloc();

    for (i = 0; i < oldnode.nlist; i++) {
        curnode = nextnode;
        nextnode = nalloc();

        curnode->pair = pairalloc();
        curnode->type = PAIR;
        if (oldnode.list[i].type == LIST) {
            curnode->pair->car = list_to_ll(oldnode.list[i]);
        }
        else
            curnode->pair->car = node_copy(oldnode.list[i]);
        curnode->pair->cdr = nextnode;
    }
    nextnode->type = NIL;
    return topnode;
}

struct node
apply_compound(struct node proc, struct node args[], int n)
{
    int i, dot;
    struct variable varlist[proc.proc->nargs];

    dot = 0;
    for (i = 0; i < proc.proc->nargs; i++) {
        if (proc.proc->symbols[i][0] == '.') {
            dot = 1;
            break;
        }
        varlist[i].symbol = proc.proc->symbols[i];
        varlist[i].value = node_copy(args[i]);
    }
    if (dot == 1) {
        struct node restargs;
        restargs.list = args+i;
        restargs.nlist = n-i;
        varlist[i].symbol = proc.proc->symbols[(i+1)];
        varlist[i].value = list_to_ll(restargs);
        i++;
    }

    extend_envlist((*proc.proc).env, varlist, i);

    return eval((*proc.proc).body, (*proc.proc).env);
}

struct node
create_procedure(char **arglist, int n, struct node body, struct environment *env)
{
    struct node proc;
    int i = 0;
    struct environment *envlist;

    //envlist = copy_environment_list(env);
    envlist = env;

    proc.proc = procalloc();
    (*proc.proc).body = body;
    (*proc.proc).nargs = n;
    for (i = 0; i < n; i++)
        strcpy((*proc.proc).symbols[i],arglist[i]);
    (*proc.proc).env = envlist;
    proc.type = PROC;
    return proc;
}



struct node
apply(struct node proc, struct node *args, int n)
{
    struct node result;
    if (primitive_proc(proc)) {
        result = apply_prim(proc, args, n);
        return result;
    }
    else
        return apply_compound(proc, args, n);
}

double
fabs(double);

struct node
apply_prim(struct node proc, struct node args[], int n)
{
    int i;
    double total;
    struct node result;
    result.type = NIL;
    total = 0;

    if (proc.symbol[0] == '+') {
        for (i = 0; i < n; i++)
            total += args[i].number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc.symbol[0] == '-') {
        total = args[0].number;
        for (i = 1; i < n; i++)
            total = total - args[i].number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc.symbol[0] == '*') {
        total = 1;
        for (i = 0; i < n; i++)
            total = total * args[i].number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc.symbol[0] == '/') {
        total = args[0].number / args[1].number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc.symbol[0] == '=') {
        int temp = args[0].number;
        total = 1;
        for (i = 0; i < n; i++)
            total = total && (temp == args[i].number);
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc.symbol[0] == '<') {
        if (args[0].number < args[1].number)
            return true_node;
        else
            return false_node;
    }
    else if (proc.symbol[0] == '>') {
        if (args[0].number > args[1].number)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc.symbol,"abs")) {
        total = fabs(args[0].number);
        result.type = NUMBER;
        result.number = total;
    }
    else if (!strcmp(proc.symbol,"cons")) {
        struct pair *newpair;
        newpair = pairalloc();
        (*newpair).car = &args[0];
        (*newpair).cdr = &args[1];
        result.type = PAIR;
        result.pair = newpair;
    }
    else if (!strcmp(proc.symbol,"car")) {
        return *(*args[0].pair).car;
    }
    else if (!strcmp(proc.symbol,"cdr")) {
        return *(*args[0].pair).cdr;
    }
    else if (!strcmp(proc.symbol,"null?")) {
        if (args[0].type == NIL)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc.symbol,"number?")) {
        if (args[0].type == NUMBER)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc.symbol,"string?")) {
        if (args[0].type == STRING)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc.symbol,"symbol?")) {
        if (args[0].type == SYMBOL)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc.symbol,"pair?")) {
        if (args[0].type == PAIR)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc.symbol,"read")) {
        result = read_list();
        return result;
    }
    else if (!strcmp(proc.symbol,"eq?")) {
        result.type = NUMBER;
        result.number = node_equal(args[0], args[1]);
    }
    else if (!strcmp(proc.symbol,"display")) {
        if (args[0].type == SYMBOL) {
            if (!strcmp(args[0].symbol, "\\n"))
                printf("\n");
        }
        else 
            minimal_print_node(args[0]);
            
        result.type = NIL;
        return result;
    }
    else if (!strcmp(proc.symbol,"begin")) {
        return args[n-1];
    }
    else if (!strcmp(proc.symbol,"apply")) {
        return apply(args[0],(args+1),(n-1));
    }
    else if (!strcmp(proc.symbol,"listconv")) {
        return *list_to_ll(args[0]);
    }
     return result;
}

primitive_proc(struct node proc)
{
    if (proc.type == SYMBOL) {
        if (proc.symbol[0] == '+')
            return 1;
        else if (proc.symbol[0] == '-')
            return 1;
        else if (proc.symbol[0] == '*')
            return 1;
        else if (proc.symbol[0] == '/')
            return 1;
        else if (proc.symbol[0] == '=')
            return 1;
        else if (proc.symbol[0] == '>')
            return 1;
        else if (proc.symbol[0] == '<')
            return 1;
        else if (!strcmp(proc.symbol,"abs"))
            return 1;
        else if (!strcmp(proc.symbol,"cons"))
            return 1;
        else if (!strcmp(proc.symbol,"car"))
            return 1;
        else if (!strcmp(proc.symbol,"cdr"))
            return 1;
        else if (!strcmp(proc.symbol,"null?"))
            return 1;
        else if (!strcmp(proc.symbol,"number?"))
            return 1;
        else if (!strcmp(proc.symbol,"symbol?"))
            return 1;
        else if (!strcmp(proc.symbol,"string?"))
            return 1;
        else if (!strcmp(proc.symbol,"pair?"))
            return 1;
        else if (!strcmp(proc.symbol,"eq?"))
            return 1;
        else if (!strcmp(proc.symbol,"read"))
            return 1;
        else if (!strcmp(proc.symbol,"display"))
            return 1;
        else if (!strcmp(proc.symbol,"begin"))
            return 1;
        else if (!strcmp(proc.symbol,"apply"))
            return 1;
        else if (!strcmp(proc.symbol,"listconv"))
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

/* garbage collection procedures */

void
garbage_collect(struct environment *);

int
add_to_pointerlist(void *[], void *[], int , int );

void
compress_allocated(void);

void
garbage_collect(struct environment *env)
{
    int i, j, usedp = 0;
    void *inuse[MAXPOINTERS];
    usedp = env_pointers(inuse, env, usedp);
    printf("pre-gc nextpointer: %d\n", nextpointer);
    for (i = 0; i < nextpointer; i++) {
  //      printf("check\n");
        if (pointer_in_list(inuse, allocated[i], usedp))
            ;
        else {
  //          printf("FREEING GARBAGE! ");
            free(allocated[i]);
            allocated[i] = NULL;
        }
    }
    compress_allocated();
    printf("post-gc nextpointer: %d\n", nextpointer);
}

void
compress_allocated(void)
{
    int i, j;
    for (i = 0; i < nextpointer; i++) { // search through all the possibly allocated pointers
        if (allocated[i]==NULL) { // if we find a null pointer
            for (j = i; j < nextpointer; j++) { // search the space past it
                if (allocated[j] != NULL) { // for a non-null pointer
                    allocated[i] = allocated[j]; // put the non-null pointer in the null space
                    allocated[j] = NULL; // mark the non-null's former space as free
                    break; // go back to searching for null pointers
                }
            }
        }
    }
    for (i = 0; allocated[i] != NULL; i++) // find the first null
        ;
    nextpointer = i; // change nextpointer allocation will start with the first null
}


int
pointer_in_list(void *mlist[], void *pointer, int mn)
{
    int i;

    for (i = 0; i < mn; i++) {
        if (mlist[i] == pointer)
            return 1;
    }
    return 0;
}

/*
struct environment {
    int status;
    struct variable vars[MAXVAR];
};
*/

/*
struct variable {
    char *symbol;
    struct node *value;
};
*/

int
env_pointers(void *mlist[], struct environment *env, int mn)
{
    int i, j;

    if (pointer_in_list(mlist, (void *) env, mn))
        return mn;

    mlist[mn++] = env;

    for (i = 0; env[i].status != 0; i++) {
        for (j=0; j < (env[i].status - 1); j++) {
            mn = node_pointers(mlist, env[i].vars[j].value, mn);
            mlist[mn++] = env[i].vars[j].symbol;
        }
    }
    return mn;
}

/*
struct node {
    int type;
    char *symbol;
    double number;
    struct node *list;
    struct procedure *proc;
    struct pair *pair;
    int nlist;
    char *string;
};
*/

int
node_pointers(void *mlist[], struct node *expr, int mn)
{
    int i;

    if (pointer_in_list(mlist, (void *) expr, mn))
        return mn;

    mlist[mn++] = expr;

    switch ((*expr).type) {
        case LIST:
            // get the pointers from each node in the list
            // this will also include the list itself at list[0] (I think)
            for (i=0; i < (*expr).nlist; i++) {
                mn = node_pointers(mlist, &((*expr).list[i]), mn);
            }
            break;
        case PROC:
            mn = proc_pointers(mlist, ((*expr).proc), mn);
            break;
        case NUMBER:
            // ain't no pointers here!
            break;
        case SYMBOL:
            mlist[mn++] = (*expr).symbol;
            break;
        case STRING:
            mlist[mn++] = (*expr).string;
            break;
        case PAIR:
            mlist[mn++] = expr->pair;
            mn = node_pointers(mlist, expr->pair->car, mn);
            mn = node_pointers(mlist, expr->pair->cdr, mn);
            break;
        case NIL:
            break;
        default:
            mlist[mn++] = (*expr).symbol;
            printf("node_pointers: unknown type %d\n", expr->type);
    }

    return mn;
}

/*
struct procedure {
    char symbols[MAXTOKEN][MAXVAR];
    struct node body;
    struct environment *env;
    int nargs;
};
*/

int
proc_pointers(void *mlist[], struct procedure *proc, int mn)
{
    int i;

    if (pointer_in_list(mlist, (void *) proc, mn))
        return mn;

    mlist[mn++] = proc;

    // process the body of the proc
    mn = node_pointers(mlist, &(*proc).body, mn);
    // process the env of the proc
    mn = env_pointers(mlist, (*proc).env, mn);
    // process the args of the proc 
    for (i=0; i < (*proc).nargs ; i++) {
        mlist[mn++] = (*proc).symbols[i];
    }
        

    return mn;
}
 
// printing

void
minimal_print_node(struct node expr)
{
    switch (expr.type) {
        case NUMBER:
            printf("%f", expr.number);
            break;
        case SYMBOL:
            printf("%s", expr.symbol);
            break;
        case PROC:
            printf("<procedure>");
            break;
        case LIST:
            print_list(expr);
            break;
        case PAIR:
            printf("<pair>");
            break;
        case STRING:
            printf("%s", expr.string);
            break;
        default:
            printf("<type: %d>", expr.type);
            break;
    }
}

void
print_node(struct node expr)
{
    int i;

    switch (expr.type) {
        case NUMBER:
            printf("\033[1;37mType:\033[0m Number\n");
            printf("\033[1;37mValue:\033[0m %f\n", expr.number);
            break;
        case SYMBOL:
            printf("\033[1;37mType:\033[0m Symbol\n");
            printf("\033[1;37mValue:\033[0m %s\n", expr.symbol);
            break;
        case PROC:
            printf("\033[1;37mType:\033[0m Procedure\n");
            printf("\033[1;37mArgs:\033[0m ");
            for (i = 0; i < (*expr.proc).nargs; i++)
                printf("%s ", (*expr.proc).symbols[i]);
            //printf("\n\033[1;37mBody:\033[0m \n");
            //print_node((*expr.proc).body);
            printf("\n\033[1;37mBody:\033[0m ");
            minimal_print_node((*expr.proc).body);
            printf("\n");
            break;
        case LIST:
            printf("\033[1;37mType:\033[0m List\n");
            printf("\033[1;37mValue:\033[0m ");
            print_list(expr);
            printf("\n");
            break;
        case STRING:
            printf("\033[1;37mType:\033[0m String\n");
            printf("\033[1;37mValue:\033[0m ");
            printf("%s\n", expr.string);
            break;
        case PAIR:
            printf("\033[1;37mType:\033[0m Pair\n");
            printf("\033[1;37mCar:\033[0m ");
            minimal_print_node(*(*expr.pair).car);
            printf("\n\033[1;37mCdr:\033[0m ");
            minimal_print_node(*(*expr.pair).cdr);
            printf("\n");
            break;
        default:
            printf("\033[1;37mType:\033[0m %d\n", expr.type);
            break;
    }
}

struct node
read_list()
{
    int c;
    struct node root;
    struct node result;

    printf("!>> ");
    c = getch(); //remove newline that led us here
    if (c == '\n')
        ;
    else
        ungetch(c);
    while ((c = getch()) != '\n') {
        if (!iswspace(c)) {
            ungetch(c);
            root = *parse_token_ll();
            //print_node(root);
        }
    } 
    ungetch('\n');
    return root;
}

//TODO: switch to passing pointers around instead of structs
//TODO: modify things so apply_proc can modify the environment? maybe?
main()
{
    int c;
    struct node root;
    struct node result;

    // the global environment
    struct environment globalenv[MAXENV] = {1, {}};

    // nil is manually bound to the empty list
    static struct node nil = {NIL};
    struct variable nilvar = {"nil", &nil};
    bind_in_current_env(globalenv, nilvar);

    // open our defaults file and read it
    FILE *readfile;
    readfile = fopen("defaults.scm", "r");
    //readfile = fopen("attempt.scm", "r");
    fseek(readfile, 0, SEEK_END);
    long pos = ftell(readfile);
    fseek(readfile, 0, SEEK_SET);
    readbuf = malloc(pos);
    fread(readbuf, pos, 1, readfile);
    fclose(readfile);
    readlength = pos - 1;

    // parse what is currently in the queue
    while (readbufp < readlength) {
        root = parse_token();
        result = eval(root, globalenv);
    }

    printf(">>");
    while ((c = getch()) != EOF) {
        if (!iswspace(c)) {
            ungetch(c);

            // parse our input
            // and get the root of the resulting s expression
            root = parse_token();

            // print the unparsed s-exp
            //print_node(root);

            // eval the s expression
            result = eval(root, globalenv);

            // print the resulting s expression
            print_node(result);

            // free pointers that cannot be accessed
            garbage_collect(globalenv);
        }
        else if (c == ' ' || c == '\t')
            ;
        else 
            printf(">>");
        if (c == ';')
            printf(">>");
    }
    return 0;
}
