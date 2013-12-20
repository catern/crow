/* insert GPLv3+ here */
/* TODO: 
   segment my code!
   switch to union types
   switch to passing pointers around
//TODO: switch to passing pointers around instead of structs
//TODO: modify things so apply_proc can modify the environment? maybe?
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "types.h"
#include "gc.h"
#include "printnode.h"
#include "parser.h"

// special nodes
struct node nil_node = { .type = NIL };

struct node true_node = { .type = NUMBER, .number = 1 };
struct node false_node = { .type = NUMBER, .number = 0 };

struct variable undefinedvar;

// end special nodes

/* EVAL STEP, BEGIN! */
struct node
eval_setcar(struct node *, struct environment *);

struct node
eval_setcdr(struct node *, struct environment *);

struct node
apply(struct node *, struct node **, int);

struct node
apply_prim(struct node *, struct node **, int);

struct node
create_procedure(char **, int, struct node *, struct environment *);

struct node
eval_if(struct node *, struct environment *);

struct node
eval_cond(struct node *, struct environment *);

struct node
eval_let(struct node *, struct environment *);

struct node
read_list(void);

struct node
eval_define(struct node *, struct environment *);

struct node
eval_lambda(struct node *, struct environment *);

struct node
eval_delay(struct node *, struct environment *);

struct node
eval_force(struct node, struct environment *);

struct node
eval_quote(struct node *, struct environment *);

struct node
eval_application(struct node *, struct environment *);

struct node
eval_load(struct node *, struct environment *);

struct variable *
lookup(struct node *, struct environment *);

struct environment *
copy_environment_list(struct environment *);

struct node *
node_copy(struct node *);

struct node *
node_copy(struct node *oldnode)
{
    struct node *newnode;
    char *string;
    int i;
    newnode = nalloc();

    switch (oldnode->type) {
        case LIST:
            newnode->list = nlistalloc();
            for (i=0; i < oldnode->nlist; i++) {
                newnode->list[i] = node_copy(oldnode->list[i]);
            }
            newnode->nlist = oldnode->nlist;
            break;
        case NUMBER:
            newnode->number = oldnode->number;
            break;
        case STRING:
            string = stralloc();
            strcpy(string, oldnode->string);
            newnode->string = string;
            break;
        case SYMBOL:
            newnode->symbol = oldnode->symbol;
            break;
        case PROC:
            newnode->proc = procalloc();
            // env
            newnode->proc->env = copy_environment_list(oldnode->proc->env);
            // args
            for (i = 0; i < oldnode->proc->nargs; i++)
                strcpy(newnode->proc->symbols[i],oldnode->proc->symbols[i]);
            newnode->proc->nargs = oldnode->proc->nargs;
            // body
            newnode->proc->body = node_copy(oldnode->proc->body);
            break;
        case PAIR:
            newnode->pair = pairalloc();
            newnode->pair->car = node_copy(oldnode->pair->car);
            newnode->pair->cdr = node_copy(oldnode->pair->cdr);
            break;
        case NIL:
            break;
        default:
            newnode->symbol = oldnode->symbol;
            printf("remember to update node_copy for type %d\n", oldnode->type);
    }
    newnode->type = oldnode->type;
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
    look = lookup(&temp, envlist);
    if (look != &undefinedvar) { 
        // if it already exists, just change the value pointer to the new one
        look->value = var.value;
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
lookup(struct node *expr, struct environment *envlist)
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
            if (strcmp(envlist[i].vars[j].symbol, expr->symbol) == 0) {
                result = &(envlist[i].vars[j]);
                return result;
            }
        }
    }
    printf("lookup: looked up a nonexistent variable: %s\n", expr->symbol);
    return &undefinedvar;
}

struct node
lookup_value(struct environment *envlist, struct node *expr)
{
    struct variable *var;
    var = lookup(expr, envlist);
    if (var != &undefinedvar)
        return *node_copy(var->value);
    else
        return nil_node;
        printf("lookup_value: looked up a nonexistent variable: %s\n", expr->symbol);
}


// evaluation

int
special_form(struct node *expr)
{
    if (expr->type == SYMBOL) {
        if (!strcmp("if", expr->symbol)) 
            return IF;
        else if (!strcmp("define", expr->symbol)) 
            return DEFINE;
        else if (!strcmp("lambda", expr->symbol)) 
            return LAMBDA;
        else if (!strcmp("delay", expr->symbol)) 
            return DELAY;
        else if (!strcmp("quote", expr->symbol)) 
            return QUOTE;
        else if (!strcmp("cond", expr->symbol)) 
            return COND;
        else if (!strcmp("let", expr->symbol)) 
            return LET;
        else if (!strcmp("set-car!", expr->symbol)) 
            return SETCAR;
        else if (!strcmp("set-cdr!", expr->symbol)) 
            return SETCDR;
        else if (!strcmp("load", expr->symbol)) 
            return LOAD;
        else
            return 0;
    }
    else
        return 0;
}


struct node
eval(struct node *expr, struct environment *env)
{
    switch (expr->type) {
        case NUMBER: // these fundamental data types are self-evaluating
        case PAIR:
        case STRING:
        case NIL:
            return *expr;
            break;
        case DELAY: // this is specially not evaluated until forced
            return *expr;
            break;
        case LIST:
            switch (special_form(expr->list[0])) { 
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
          // both of these can be a good place TODO next
            if (primitive_proc(expr))
                return *expr;
            else {
                return lookup_value(env, expr);
            }
            break;
        default:
            printf("eval: Unknown expression type %d;\n", expr->type);
    }
}

struct node
eval_setcar(struct node *expr, struct environment *env)
{
    struct variable *var;
    var = lookup(expr->list[1],env);
    struct node newcar = eval(expr->list[2], env);
    var->value->pair->car = node_copy(&newcar);
    return nil_node;
}

struct node
eval_setcdr(struct node *expr, struct environment *env)
{
    struct variable *var;
    var = lookup(expr->list[1],env);
    struct node newcdr = eval(expr->list[2], env);
    var->value->pair->cdr = node_copy(&newcdr);
    return nil_node;
}

struct node
eval_let(struct node *expr, struct environment *env)
{
    struct environment *newenv;
    struct variable varlist[expr->list[1]->nlist];
    struct node body, cur;
    newenv = copy_environment_list(env);
    int i;

    for (i = 0; i < expr->list[1]->nlist; i++) {
        varlist[i].symbol = expr->list[1]->list[i]->list[0]->symbol;
        cur = eval(expr->list[1]->list[i]->list[1], env);
        varlist[i].value = node_copy(&cur);
    }

    struct node *allocbody;
    extend_envlist(newenv, varlist, i);
    expr->list[1] = nalloc();
    expr->list[1]->type = SYMBOL;
    expr->list[1]->symbol = tokenalloc();
    strcpy(expr->list[1]->symbol,"begin");
    body.type = LIST;
    body.list = (expr->list+1);
    body.nlist = expr->nlist-1;
    allocbody = node_copy(&body);
    // body.list will get gc'd if we don't copy it, since it isn't 
    // actually allocated, it's an allocated_pointer+1

    return eval(allocbody, newenv);
}

struct node
eval_cond(struct node *expr, struct environment *env)
{
    int i;
    for (i = 1; i < expr->nlist; i++) {
        if ((expr->list[i]->list[0]->type == SYMBOL && 
                    !strcmp(expr->list[i]->list[0]->symbol,"else")) ||
            istrue(expr->list[i]->list[0], env)) {
            return eval(expr->list[i]->list[1], env);
        }
    }
}

struct node
eval_quote(struct node *expr, struct environment *env)
{
    return *expr->list[1];
}

struct node
eval_delay(struct node *expr, struct environment *env)
{
    return create_procedure(NULL, 0, expr->list[1], env);
}

struct node
eval_lambda(struct node *expr, struct environment *env)
{
    char *arglist[expr->list[1]->nlist];
    int i, dot;
    struct node proc;

    // copies the tokens from the list in the second position to another array
    for (i = 0; i < expr->list[1]->nlist; i++) {
        arglist[i] = tokenalloc();
        strcpy(arglist[i], expr->list[1]->list[i]->symbol);
    }

    proc = create_procedure(arglist, i, expr->list[2], env);
    return proc;
}

struct node
eval_define(struct node *expr, struct environment *env)
{
    struct variable var;
    struct node *varvalue;
    varvalue = nalloc();

    if (expr->list[1]->type == LIST) {
        var.symbol = expr->list[1]->list[0]->symbol;
        var.value = varvalue; 
        bind_in_current_env(env, var);

        struct node body;
        char *arglist[MAXVAR];
        int i;
        for (i = 1; i < expr->list[1]->nlist; i++) {
          arglist[(i-1)] = tokenalloc();
          strcpy(arglist[(i-1)], expr->list[1]->list[i]->symbol);
        }
        struct node *allocbody;
        expr->list[1] = nalloc();
        expr->list[1]->type = SYMBOL;
        expr->list[1]->symbol = tokenalloc();
        strcpy(expr->list[1]->symbol,"begin");
        body.type = LIST;
        body.nlist = expr->nlist-1;
        body.list = expr->list+1;

        allocbody = node_copy(&body); 
        // body.list will get gc'd if we don't copy it, since it isn't 
        // actually allocated, it's an allocated_pointer+1

        (*varvalue) = create_procedure(arglist, (i - 1), allocbody, env);
    }
    else {
        var.symbol = expr->list[1]->symbol;
        var.value = varvalue; 
        bind_in_current_env(env, var);

        (*varvalue) = eval(expr->list[2], env);
    }
    return (*varvalue);
}

struct node
eval_load(struct node *expr, struct environment *env)
{
    if (expr->list[1]->type == STRING) {
        char* filename;
        filename = expr->list[1]->string;
        eval(parse_file(filename), env);
    }
    return nil_node;
}

int
istrue(struct node *expr, struct environment *env)
{
    struct node val = eval(expr, env);

    if (val.type == NUMBER && val.number == 0)
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
eval_if(struct node *expr, struct environment *env)
{
    if (istrue(expr->list[1], env))
        return eval(expr->list[2], env);
    else
        return eval(expr->list[3], env);
}

struct node
eval_application(struct node *expr, struct environment *env)
{
    struct node proc;
    struct node cur;
    int i;

    for (i = 0; i < expr->nlist; i++) {
        cur = eval(expr->list[i], env);
        expr->list[i] = nalloc();
        *expr->list[i] = cur;
    }

    return apply(expr->list[0], (expr->list+1), (i-1));
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
        if (oldnode.list[i]->type == LIST) {
            curnode->pair->car = list_to_ll(*oldnode.list[i]);
        }
        else
            curnode->pair->car = node_copy(oldnode.list[i]);
        curnode->pair->cdr = nextnode;
    }
    nextnode->type = NIL;
    return topnode;
}

struct node
apply_compound(struct node *proc, struct node *args[], int n)
{
    int i, dot;
    struct variable varlist[proc->proc->nargs];

    dot = 0;
    for (i = 0; i < proc->proc->nargs; i++) {
        if (proc->proc->symbols[i][0] == '.') {
            dot = 1;
            break;
        }
        varlist[i].symbol = proc->proc->symbols[i];
        varlist[i].value = node_copy(args[i]);
    }
    if (dot == 1) {
        struct node restargs;
        restargs.list = args+i;
        restargs.nlist = n-i;
        varlist[i].symbol = proc->proc->symbols[(i+1)];
        varlist[i].value = list_to_ll(restargs);
        i++;
    }

    extend_envlist(proc->proc->env, varlist, i);

    return eval(proc->proc->body, proc->proc->env);
}

struct node
create_procedure(char **arglist, int n, struct node *body, struct environment *env)
{
    struct node proc;
    int i = 0;
    struct environment *envlist;

    //envlist = copy_environment_list(env);
    envlist = env;

    proc.proc = procalloc();
    proc.proc->body = node_copy(body);
    proc.proc->nargs = n;
    for (i = 0; i < n; i++)
        strcpy(proc.proc->symbols[i],arglist[i]);
    proc.proc->env = envlist;
    proc.type = PROC;
    return proc;
}

struct node
apply(struct node *proc, struct node **args, int n)
{
    struct node result;
    if (primitive_proc(proc)) {
        result = apply_prim(proc, args, n);
        return result;
    }
    else
        return apply_compound(proc, args, n);
}

struct node
apply_prim(struct node *proc, struct node *args[], int n)
{
    int i;
    double total;
    struct node result;
    result.type = NIL;
    total = 0;

    if (proc->symbol[0] == '+') {
        for (i = 0; i < n; i++)
            total += args[i]->number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc->symbol[0] == '-') {
        total = args[0]->number;
        for (i = 1; i < n; i++)
            total = total - args[i]->number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc->symbol[0] == '*') {
        total = 1;
        for (i = 0; i < n; i++)
            total = total * args[i]->number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc->symbol[0] == '/') {
        total = args[0]->number / args[1]->number;
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc->symbol[0] == '=') {
        int temp = args[0]->number;
        total = 1;
        for (i = 0; i < n; i++)
            total = total && (temp == args[i]->number);
        result.type = NUMBER;
        result.number = total;
    }
    else if (proc->symbol[0] == '<') {
        if (args[0]->number < args[1]->number)
            return true_node;
        else
            return false_node;
    }
    else if (proc->symbol[0] == '>') {
        if (args[0]->number > args[1]->number)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc->symbol,"abs")) {
        total = fabs(args[0]->number);
        result.type = NUMBER;
        result.number = total;
    }
    else if (!strcmp(proc->symbol,"cons")) {
        struct pair *newpair;
        newpair = pairalloc();
        newpair->car = args[0];
        newpair->cdr = args[1];
        result.type = PAIR;
        result.pair = newpair;
    }
    else if (!strcmp(proc->symbol,"car")) {
        return *args[0]->pair->car;
    }
    else if (!strcmp(proc->symbol,"cdr")) {
        return *args[0]->pair->cdr;
    }
    else if (!strcmp(proc->symbol,"null?")) {
        if (args[0]->type == NIL)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc->symbol,"number?")) {
        if (args[0]->type == NUMBER)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc->symbol,"string?")) {
        if (args[0]->type == STRING)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc->symbol,"symbol?")) {
        if (args[0]->type == SYMBOL)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc->symbol,"pair?")) {
        if (args[0]->type == PAIR)
            return true_node;
        else
            return false_node;
    }
    else if (!strcmp(proc->symbol,"read")) {
        result = read_list();
        return result;
    }
    else if (!strcmp(proc->symbol,"eq?")) {
        result.type = NUMBER;
        result.number = node_equal(*args[0], *args[1]);
    }
    else if (!strcmp(proc->symbol,"display")) {
        if (args[0]->type == SYMBOL) {
            if (!strcmp(args[0]->symbol, "\\n"))
                printf("\n");
        }
        else 
            minimal_print_node(args[0]);
            
        result.type = NIL;
        return result;
    }
    else if (!strcmp(proc->symbol,"begin")) {
        return *args[n-1];
    }
    else if (!strcmp(proc->symbol,"apply")) {
        return apply(args[0],(args+1),(n-1));
    }
    else if (!strcmp(proc->symbol,"listconv")) {
        return *list_to_ll(*args[0]);
    }
     return result;
}

primitive_proc(struct node *proc)
{
    if (proc->type == SYMBOL) {
        if (proc->symbol[0] == '+')
            return 1;
        else if (proc->symbol[0] == '-')
            return 1;
        else if (proc->symbol[0] == '*')
            return 1;
        else if (proc->symbol[0] == '/')
            return 1;
        else if (proc->symbol[0] == '=')
            return 1;
        else if (proc->symbol[0] == '>')
            return 1;
        else if (proc->symbol[0] == '<')
            return 1;
        else if (!strcmp(proc->symbol,"abs"))
            return 1;
        else if (!strcmp(proc->symbol,"cons"))
            return 1;
        else if (!strcmp(proc->symbol,"car"))
            return 1;
        else if (!strcmp(proc->symbol,"cdr"))
            return 1;
        else if (!strcmp(proc->symbol,"null?"))
            return 1;
        else if (!strcmp(proc->symbol,"number?"))
            return 1;
        else if (!strcmp(proc->symbol,"symbol?"))
            return 1;
        else if (!strcmp(proc->symbol,"string?"))
            return 1;
        else if (!strcmp(proc->symbol,"pair?"))
            return 1;
        else if (!strcmp(proc->symbol,"eq?"))
            return 1;
        else if (!strcmp(proc->symbol,"read"))
            return 1;
        else if (!strcmp(proc->symbol,"display"))
            return 1;
        else if (!strcmp(proc->symbol,"begin"))
            return 1;
        else if (!strcmp(proc->symbol,"apply"))
            return 1;
        else if (!strcmp(proc->symbol,"listconv"))
            return 1;
        else
            return 0;
    }
    else
        return 0;
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
            //print_node(&root);
        }
    } 
    ungetch('\n');
    return root;
}

main()
{
    int c;
    struct node *root;
    struct node result;

    // the global environment
    struct environment globalenv[MAXENV] = {1, {}};

    // nil is manually bound to the empty list
    struct variable nilvar = {"nil", &nil_node};
    bind_in_current_env(globalenv, nilvar);

    // get our defaults
    eval(parse_file("defaults.scm"), globalenv);

    printf(">>");
    while ((c = getch()) != EOF) {
        if (!iswspace(c)) {
            ungetch(c);

            // parse our input
            // and get the root of the resulting s expression
            root = parse_token();

            // print the unevaled s-exp
            //print_node(root);

            // eval the s expression
            result = eval(root, globalenv);

            // print the resulting s expression
            print_node(&result);

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
