/* TODO:
   remove use of copy_environment_list
   use glib lists for environment lists
   switch to union types
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "types.h"
#include "gc.h"
#include "printnode.h"
#include "parser.h"

/* EVAL STEP, BEGIN! */
struct variable *
lookup(const gchar *symbol, struct environment **envlist);

struct environment **
copy_environment_list(struct environment **oldenv);

struct node *
node_copy(struct node *oldnode);

struct node *
read_list(void);

struct node *
eval_if(struct node *expr, struct environment **env);

struct node *
eval_cond(struct node *expr, struct environment **env);

struct node *
eval_let(struct node *expr, struct environment **env);

struct node *
eval_setcar(struct node *expr, struct environment **env);

struct node *
eval_setcdr(struct node *expr, struct environment **env);

struct node *
eval_define(struct node *expr, struct environment **env);

struct node *
eval_lambda(struct node *expr, struct environment **env);

struct node *
eval_delay(struct node *expr, struct environment **env);

struct node *
eval_quote(struct node *expr, struct environment **env);

struct node *
eval_application(struct node *expr, struct environment **env);

struct node *
eval_load(struct node *expr, struct environment **env);

struct node *
apply(struct node *proc, struct node **args, int n);

struct node *
apply_compound(struct node *proc, struct node *args[], int n);

struct node *
apply_prim(struct node *proc, struct node *args[], int n);

// copy a node and everything it references
struct node *
node_copy(struct node *oldnode)
{
    struct node *newnode;
    int i;

    switch (oldnode->type) {
    case LIST: {
        newnode = list_to_node(nlistalloc(),oldnode->nlist);
        for (i=0; i < oldnode->nlist; i++) {
            newnode->list[i] = node_copy(oldnode->list[i]);
        }
        break;
    }
    case NUMBER: {
        newnode = double_to_node(oldnode->number);
        break;
    }
    case STRING: {
        newnode = string_to_node(oldnode->string->str);
        break;
    }
    case SYMBOL: {
        newnode = symbol_to_node(oldnode->symbol);
        break;
    }
    case PROC: {
        newnode =
            procedure_to_node(oldnode->proc->symbols,
                              oldnode->proc->nargs,
                              node_copy(oldnode->proc->body),
                              copy_environment_list(oldnode->proc->env));
        break;
    }
    case PAIR: {
        newnode = pair_to_node(node_copy(oldnode->pair->car), node_copy(oldnode->pair->cdr));
        break;
    }
    case NIL: {
        newnode = nil_alloc();
        break;
    }
    default: {
        newnode = oldnode;
        printf("node_copy: unknown type %d\n", oldnode->type);
    }
    }
    return newnode;
}


// environments

// copy the n variables in a varlist to a new environment frame
void
extend_envlist(struct environment **envlist, struct variable **varlist, int n)
{
    int i;
    struct environment *newenv = envalloc();
    newenv->vars = varlistalloc();

    // setup the new env
    for (i=0; i < n; i++) {
        newenv->vars[i] = varlist[i];
    }

    //find the end of the envlist
    for (i=0; envlist[i] != NULL; i++)
        ;
    // set the end of the envlist to newenv
    envlist[i] = newenv;
    envlist[i+1] = NULL;
}

// copy the environment list and its references, but not the
// referenced nodes
struct environment **
copy_environment_list(struct environment **oldenv)
{
    int i, j;
    struct environment **newenv;
    newenv = envlistalloc();

    for (i = 0; oldenv[i] != NULL; i++) {
        newenv[i] = envalloc();
        newenv[i]->vars = varlistalloc();

        for (j=0; oldenv[i]->vars[j] != NULL; j++) {
            newenv[i]->vars[j] = varalloc();
            newenv[i]->vars[j]->symbol = oldenv[i]->vars[j]->symbol;
            newenv[i]->vars[j]->value = oldenv[i]->vars[j]->value;
        }
    }
    newenv[i] = NULL;
    return newenv;
}

// make a binding in the last environment frame
void
bind_in_current_env(struct environment **envlist, const gchar *symbol, struct node *value)
{
    int i, j;

    // first look it up
    struct variable *look = lookup(symbol, envlist);

    if (look != NULL) {
        // if it already exists, just change the value pointer to the new one
        look->value = value;
    } else {
        // it doesn't exist
        for (i=0; envlist[i] != NULL; i++)
            ;
        // bind in the current environment, not a new one, so use i-1
        for (j=0; envlist[i-1]->vars[j] != NULL; j++)
            ;
        envlist[i-1]->vars[j] = varalloc();
        envlist[i-1]->vars[j]->symbol = symbol;
        envlist[i-1]->vars[j]->value = value;
    }
}

// look up a variable, return NULL if not found
struct variable *
lookup(const gchar *symbol, struct environment **envlist)
{
    int i, j;
    struct variable *result;

    // find the end of the envlist
    for (i=0; envlist[i] != NULL; i++)
        ;
    //run through it backwards (skipping the null env)
    for (i = i-1; i >= 0; i--) {
        // scan through varlist until reaching end
        for (j=0; envlist[i]->vars[j] != NULL; j++) {
            // if the symbols match, you've found the variable
            if (envlist[i]->vars[j]->symbol == symbol) {
                return envlist[i]->vars[j];
            }
        }
    }
#ifdef DEBUG
    printf("lookup: looked up a nonexistent variable: %s\n", symbol);
#endif
    return NULL;
}

// look up a variable's value immutably (by copying it before returning)
struct node *
lookup_value(struct environment **envlist, struct node *expr)
{
    struct variable *var = lookup(expr->symbol, envlist);
    struct node *result;
    if (var != NULL) {
        result = node_copy(var->value);
    } else {
        printf("lookup_value: looked up a nonexistent variable: %s\n", expr->symbol);
        result = nil_alloc();
    }
    return result;
}

// evaluation
int
special_form(struct node *expr)
{
    if (expr->type == SYMBOL) {
        if (expr->symbol == ("if")) {
            return IF;
        } else if (expr->symbol == ("define")) {
            return DEFINE;
        } else if (expr->symbol == ("lambda")) {
            return LAMBDA;
        } else if (expr->symbol == ("delay")) {
            return DELAY;
        } else if (expr->symbol == ("quote")) {
            return QUOTE;
        } else if (expr->symbol == ("cond")) {
            return COND;
        } else if (expr->symbol == ("let")) {
            return LET;
        } else if (expr->symbol == ("set-car!")) {
            return SETCAR;
        } else if (expr->symbol == ("set-cdr!")) {
            return SETCDR;
        } else if (expr->symbol == ("load")) {
            return LOAD;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

struct node *
eval(struct node *expr, struct environment **env)
{
    switch (expr->type) {
    // these fundamental data types are self-evaluating
    case NUMBER:
    case PAIR:
    case STRING:
    case NIL: {
        return expr;
        break;
    }
    // this is specially not evaluated until forced
    case DELAY: {
        return expr;
        break;
    }
    case LIST: {
        switch (special_form(expr->list[0])) {
        // some special syntax needs special handling
        // with these forms, we can't simply eval all the arguments
        case IF: {
            return eval_if(expr, env);
            break;
        }
        case COND: {
            return eval_cond(expr, env);
            break;
        }
        case DEFINE: {
            return eval_define(expr, env);
            break;
        }
        case LAMBDA: {
            return eval_lambda(expr, env);
            break;
        }
        case DELAY: {
            return eval_delay(expr, env);
            break;
        }
        case QUOTE: {
            return eval_quote(expr, env);
            break;
        }
        case LET: {
            return eval_let(expr, env);
            break;
        }
        case SETCAR: {
            return eval_setcar(expr, env);
            break;
        }
        case SETCDR: {
            return eval_setcdr(expr, env);
            break;
        }
        case LOAD: {
            return eval_load(expr, env);
            break;
        }
        // by default interpret the list as a function application
        default: {
            return eval_application(expr, env);
        }
        }
        break;
    }
    case SYMBOL: {
        // if the symbol is a primitive, it's "self-evaluating" from
        // our perspective
        if (primitive_proc(expr)) {
            return expr;
        }
        // otherwise, look up the symbol's value
        else {
            return lookup_value(env, expr);
        }
        break;
    }
    default:
        printf("eval: Unknown expression type %d;\n", expr->type);
    }
}

// proper form: (setcar somesymbol)
// requires: somesymbol corresponds to a variable with a pair value
struct node *
eval_setcar(struct node *expr, struct environment **env)
{
    struct variable *var = lookup(expr->list[1]->symbol,env);

    var->value->pair->car = eval(expr->list[2], env);

    return nil_alloc();
}

// proper form: (setcdr somesymbol)
// requires: somesymbol corresponds to a variable with a pair value
struct node *
eval_setcdr(struct node *expr, struct environment **env)
{
    struct variable *var = lookup(expr->list[1]->symbol,env);

    var->value->pair->cdr = eval(expr->list[2], env);

    return nil_alloc();
}

// proper form: (let ((var1 val1) (var2 val2)) expression)
struct node *
eval_let(struct node *expr, struct environment **env)
{
    int i;
    // create variables defined by let
    struct variable *varlist[expr->list[1]->nlist];
    for (i = 0; i < expr->list[1]->nlist; i++) {
        varlist[i] = varalloc();
        varlist[i]->symbol = expr->list[1]->list[i]->list[0]->symbol;
        varlist[i]->value = eval(expr->list[1]->list[i]->list[1], env);
    }
    // create new environment with these bindings
    struct environment **newenv = copy_environment_list(env);
    extend_envlist(newenv, varlist, i);

    // create node to represent the sequence of expressions in the rest of the let
    struct node *body = list_to_node(nlistalloc(), expr->nlist - 1);

    // put begin node at the beginning of list
    body->list[0] = symbol_to_node(("begin"));

    // rest of list is the expressions from the let
    for (i = 1; i+1 < expr->nlist; i++) {
        body->list[i] = expr->list[i+1];
    }
    body->nlist = expr->nlist - 1;

    // evaluate the sequence of expressions in the rest of the let, in
    // the new environment
    return eval(body, newenv);
}

// form: (cond ((predicate1) val1) ((predicate2) val2) (else val3) )
struct node *
eval_cond(struct node *expr, struct environment **env)
{
    int i;
    for (i = 1; i < expr->nlist; i++) {
        // if the predicate is true, or this is an else-clause, return the corresponding value
        if ((expr->list[i]->list[0]->type == SYMBOL && (expr->list[i]->list[0]->symbol == ("else")))
                || istrue(eval(expr->list[i]->list[0], env))) {
            return eval(expr->list[i]->list[1], env);
        }
    }
    return nil_alloc();
}

// form: (quote foo)
// returns just the value passed in (a list of data, perhaps)
struct node *
eval_quote(struct node *expr, struct environment **env)
{
    return expr->list[1];
}

// form: (delay foo)
// delays evaluation of foo until (force (delay foo)) is called
struct node *
eval_delay(struct node *expr, struct environment **env)
{
    // implemented as a zero-arg procedure
    return procedure_to_node(NULL, 0, expr->list[1], env);
}

// form: (lambda (arg1 arg2 arg3) (expr arg1 arg2 arg3))
// returns an anonymous function
struct node *
eval_lambda(struct node *expr, struct environment **env)
{
    const gchar *arglist[expr->list[1]->nlist];
    int i;

    // collects the tokens from the list in the second position in another array
    for (i = 0; i < expr->list[1]->nlist; i++) {
        arglist[i] = expr->list[1]->list[i]->symbol;
    }

    return procedure_to_node(arglist, i, expr->list[2], env);
}

// defines a variable or recursive function
// form: (define varname var)
// alt form: (define (funname arg1 arg2 arg3) (expr arg1 arg2 arg3))
// recursive functions must be declared with this form, otherwise they
// won't be in their own scope and can't call themselves
struct node *
eval_define(struct node *expr, struct environment **env)
{
    struct node *varvalue;
    const gchar *name;

    // function
    // form: (define (funname arg1 arg2 arg3) (expr arg1 arg2 arg3))
    if (expr->list[1]->type == LIST) {
        name = expr->list[1]->list[0]->symbol;

        // argument list
        const gchar *arglist[expr->list[1]->nlist - 1];
        int i,j;
        for (i = 0; i+1 < expr->list[1]->nlist; i++) {
            arglist[i] = expr->list[1]->list[i+1]->symbol;
        }

        // expression body
        // create node to represent the expressions in the rest of the define
        struct node *body = list_to_node(nlistalloc(), expr->nlist - 1);

        // put begin node at the beginning of list
        body->list[0] = symbol_to_node(("begin"));

        // rest of list is the expressions from the let
        for (j = 1; j+1 < expr->nlist; j++) {
            body->list[j] = expr->list[j+1];
        }

        // environment
        // lexical scoping: procedure is a closure and has its own env
        /* struct environment **procenv = copy_environment_list(env); */

        /* varvalue = procedure_to_node(arglist, i, body, procenv); */

        /* bind_in_current_env(procenv, name, varvalue); */

        // dynamic scoping: procedure sees the global env
        varvalue = procedure_to_node(arglist, i, body, env);
    }
    // variable
    // form: (define varname var)
    else {
        name = expr->list[1]->symbol;
        varvalue = eval(expr->list[2], env);
    }
    bind_in_current_env(env, name, varvalue);
    return varvalue;
}

// form: (load filename)
// evals that file
struct node *
eval_load(struct node *expr, struct environment **env)
{
    if (expr->list[1]->type == STRING) {
        char* filename = expr->list[1]->string->str;
        return eval(parse_file(filename), env);
    }
    return nil_alloc();
}

// returns the truth-value of the expression
gboolean
istrue(struct node *expr)
{
    if (expr->type == NUMBER && expr->number == 0) {
        return 0;
    } else {
        return 1;
    }
}

// returns equality-status of two nodes
int
node_equal(struct node *n1, struct node *n2)
{
    if (n1->type == SYMBOL && n2->type == SYMBOL) {
        if (n1->symbol == n2->symbol) {
            return 1;
        }
    } else if (n1->type == NUMBER && n2->type == NUMBER) {
        if (n1->number == n2->number) {
            return 1;
        }
    } else if (n1->type == NIL && n2->type == NIL) {
        return 1;
    }
    return 0;
}

// form: (if predicate trueval falseval)
struct node *
eval_if(struct node *expr, struct environment **env)
{
    if (istrue(eval(expr->list[1], env))) {
        return eval(expr->list[2], env);
    } else {
        return eval(expr->list[3], env);
    }
}

// evaluate everything in the list, then apply the first node as a
// procedure to the rest
struct node *
eval_application(struct node *expr, struct environment **env)
{
    int i;
    for (i = 0; i < expr->nlist; i++) {
        expr->list[i] = eval(expr->list[i], env);
    }

    return apply(expr->list[0], (expr->list+1), (i-1));
}

// convert an internal array-list to a linked-list of pairs
struct node *
list_to_ll(struct node **nodelist, int n)
{
    struct node *topnode, *curnode, *nextnode;
    int i;

    if (n == 0) {
        return nil_alloc();
    }

    topnode = curnode = pair_to_node(nodelist[0], NULL);
    for (i = 1; i < n; i++) {
        curnode->pair->cdr = pair_to_node(nodelist[i], NULL);
        curnode = curnode->pair->cdr;
    }
    curnode->pair->cdr = nil_alloc();
    return topnode;
}

/* APPLY STEP, BEGIN! */

// if the first node represents a primitive, treat it specially,
// otherwise assume it's a compound, user-defined function
struct node *
apply(struct node *proc, struct node **args, int n)
{
    if (primitive_proc(proc)) {
        return apply_prim(proc, args, n);
    } else {
        return apply_compound(proc, args, n);
    }
}


struct node *
apply_compound(struct node *proc, struct node *args[], int n)
{
    int i, dot;
    struct variable *varlist[proc->proc->nargs];

    dot = 0;
    // put each arg into a variable with the name defined in proc
    for (i = 0; i < proc->proc->nargs; i++) {
        if (proc->proc->symbols[i][0] == '.') {
            dot = 1;
            break;
        }
        varlist[i] = varalloc();
        varlist[i]->symbol = proc->proc->symbols[i];
        varlist[i]->value = args[i];
    }
    // special form of function definition
    // (lambda (arg1 arg2 . restargs)) or (define (f arg1 arg2 . restargs))
    // will put args 3 and up into a linked list in restargs
    if (dot == 1) {
        varlist[i] = varalloc();
        varlist[i]->symbol = proc->proc->symbols[i+1];
        varlist[i]->value = list_to_ll(args+i, n-i);
        i++;
    }

    // add the variable args to the environment
    extend_envlist(proc->proc->env, varlist, i);

    // evaluate the procedure's body in this new environment
    return eval(proc->proc->body, proc->proc->env);
}

// apply a primitive symbol
struct node *
apply_prim(struct node *proc, struct node *args[], int n)
{
    int i;
    double total;
    struct node *result;

    if (proc->symbol == ("+")) {
        total = 0;
        for (i = 0; i < n; i++) {
            total += args[i]->number;
        }
        result = double_to_node(total);
    } else if (proc->symbol == ("-")) {
        total = args[0]->number;
        for (i = 1; i < n; i++) {
            total = total - args[i]->number;
        }
        result = double_to_node(total);
    } else if (proc->symbol == ("*")) {
        total = 1;
        for (i = 0; i < n; i++) {
            total = total * args[i]->number;
        }
        result = double_to_node(total);
    } else if (proc->symbol == ("/")) {
        total = args[0]->number;
        for (i = 1; i < n; i++) {
            total = total / args[i]->number;
        }
        result = double_to_node(total);
    } else if (proc->symbol == ("=")) {
        total = 1;
        for (i = 0; i < n; i++) {
            total = total && (args[0]->number == args[i]->number);
        }
        result = bool_to_node(total);
    } else if (proc->symbol == ("<")) {
        result = bool_to_node((args[0]->number < args[1]->number));
    } else if (proc->symbol == (">")) {
        result = bool_to_node((args[0]->number > args[1]->number));
    } else if (proc->symbol == ("abs")) {
        total = fabs(args[0]->number);
        result = double_to_node(total);
    } else if (proc->symbol == ("cons")) {
        result = pair_to_node(args[0],args[1]);
    } else if (proc->symbol == ("car")) {
        result = args[0]->pair->car;
    } else if (proc->symbol == ("cdr")) {
        result = args[0]->pair->cdr;
    } else if (proc->symbol == ("null?")) {
        result = bool_to_node((args[0]->type == NIL));
    } else if (proc->symbol == ("number?")) {
        result = bool_to_node((args[0]->type == NUMBER));
    } else if (proc->symbol == ("string?")) {
        result = bool_to_node((args[0]->type == STRING));
    } else if (proc->symbol == ("symbol?")) {
        result = bool_to_node((args[0]->type == SYMBOL));
    } else if (proc->symbol == ("pair?")) {
        result = bool_to_node((args[0]->type == PAIR));
    } else if (proc->symbol == ("read")) {
        result = read_list();
    } else if (proc->symbol == ("eq?")) {
        result = bool_to_node(node_equal(args[0], args[1]));
    } else if (proc->symbol == ("display")) {
        minimal_print_node(args[0]);
        result = nil_alloc();
    } else if (proc->symbol == ("begin")) {
        result = args[n-1];
    } else if (proc->symbol == ("apply")) {
        result = apply(args[0],(args+1),(n-1));
    } else if (proc->symbol == ("listconv")) {
        result = list_to_ll(args, n);
    } else {
        result = nil_alloc();
    }
    return result;
}

// should really convert this into an array which I iterate through
primitive_proc(struct node *proc)
{
    if (proc->type == SYMBOL) {
        if (proc->symbol == ("+")) {
            return 1;
        } else if (proc->symbol == ("-")) {
            return 1;
        } else if (proc->symbol == ("*")) {
            return 1;
        } else if (proc->symbol == ("/")) {
            return 1;
        } else if (proc->symbol == ("=")) {
            return 1;
        } else if (proc->symbol == (">")) {
            return 1;
        } else if (proc->symbol == ("<")) {
            return 1;
        } else if (proc->symbol == ("abs")) {
            return 1;
        } else if (proc->symbol == ("cons")) {
            return 1;
        } else if (proc->symbol == ("car")) {
            return 1;
        } else if (proc->symbol == ("cdr")) {
            return 1;
        } else if (proc->symbol == ("null?")) {
            return 1;
        } else if (proc->symbol == ("number?")) {
            return 1;
        } else if (proc->symbol == ("symbol?")) {
            return 1;
        } else if (proc->symbol == ("string?")) {
            return 1;
        } else if (proc->symbol == ("pair?")) {
            return 1;
        } else if (proc->symbol == ("eq?")) {
            return 1;
        } else if (proc->symbol == ("read")) {
            return 1;
        } else if (proc->symbol == ("display")) {
            return 1;
        } else if (proc->symbol == ("begin")) {
            return 1;
        } else if (proc->symbol == ("apply")) {
            return 1;
        } else if (proc->symbol == ("listconv")) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

struct node *
read_list()
{
    int c;
    struct node *root;

    printf("!>> ");

    // avoid starting with a newline
    if ((c = getch()) == '\n') {
        ;
    } else {
        ungetch(c);
    }

    while ((c = getch()) != '\n') {
        if (!iswspace(c)) {
            ungetch(c);
            root = parse_ll();
            /* print_node(root); */
        }
    }
    ungetch('\n');
    return root;
}

// intern these strings for fast comparison
// we intern all input symbols, so we can just do pointer equality
void
init_intern()
{
    // primitives
    g_intern_static_string("+");
    g_intern_static_string("-");
    g_intern_static_string("*");
    g_intern_static_string("/");
    g_intern_static_string("=");
    g_intern_static_string(">");
    g_intern_static_string("<");
    g_intern_static_string("abs");
    g_intern_static_string("cons");
    g_intern_static_string("car");
    g_intern_static_string("cdr");
    g_intern_static_string("null?");
    g_intern_static_string("number?");
    g_intern_static_string("symbol?");
    g_intern_static_string("string?");
    g_intern_static_string("pair?");
    g_intern_static_string("eq?");
    g_intern_static_string("read");
    g_intern_static_string("display");
    g_intern_static_string("begin");
    g_intern_static_string("apply");
    g_intern_static_string("listconv");

    // special forms
    g_intern_static_string("if");
    g_intern_static_string("define");
    g_intern_static_string("lambda");
    g_intern_static_string("delay");
    g_intern_static_string("quote");
    g_intern_static_string("cond");
    g_intern_static_string("let");
    g_intern_static_string("set-car!");
    g_intern_static_string("set-cdr!");
    g_intern_static_string("load");

    // misc
    g_intern_static_string("else");
    g_intern_static_string("nil");

}

main()
{
    int c;

    // intern all our internally used strings
    init_intern();

    // set up the empty global environment
    struct environment **globalenv = envlistalloc();
    globalenv[0] = envalloc();
    globalenv[0]->vars = varlistalloc();

    // the symbol nil is manually bound to the value nil
    struct node *nil = nil_alloc();
    bind_in_current_env(globalenv, ("nil"), nil);

    // get our defaults
    eval(parse_file("defaults.scm"), globalenv);
    /* eval(parse_file("example.scm"), globalenv); */

    printf(">>");

    // main REPL
    while ((c = getch()) != EOF) {
        if (!iswspace(c)) {
            ungetch(c);

            // READ and parse our input
            /* struct node *root = parse(); */

            // print the unevaled s-expression for parser debugging
            //print_node(root);

            // EVAL the s-expression
            /* struct node *result = eval(root, globalenv); */

            // PRINT the resulting s-expression
            /* print_node(result); */

            // do read, eval, print more concisely
            print_node(eval(parse(), globalenv));

            // free pointers that cannot be accessed
            garbage_collect(globalenv);

        } else if (c == '\n') {
            // new line of input, print our prompt
            printf(">>");
        }

        if (c == ';') {
            // we just saw a comment. this means we'll never see the
            // inevitable newline, so print our prompt right now
            printf(">>");
        }
        // and finally, LOOP
    }
    return 0;
}
