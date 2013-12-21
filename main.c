/* insert GPLv3+ here */
/* TODO: 
   gliiiiiiiiiiib
   switch to union types (requires major refactoring)
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
struct node *
eval_setcar(struct node *, struct environment **);

struct node *
eval_setcdr(struct node *, struct environment **);

struct node *
apply(struct node *, struct node **, int);

struct node *
apply_prim(struct node *, struct node **, int);

struct node *
eval_if(struct node *, struct environment **);

struct node *
eval_cond(struct node *, struct environment **);

struct node *
eval_let(struct node *, struct environment **);

struct node *
read_list(void);

struct node *
eval_define(struct node *, struct environment **); 

struct node *
eval_lambda(struct node *, struct environment **);

struct node *
eval_delay(struct node *, struct environment **);

struct node *
eval_quote(struct node *, struct environment **);

struct node *
eval_application(struct node *, struct environment **);

struct node *
eval_load(struct node *, struct environment **);

struct variable *
lookup(char *, struct environment **);

struct environment **
copy_environment_list(struct environment **);

struct node *
node_copy(struct node *);

struct node *
node_copy(struct node *oldnode)
{
  struct node *newnode;
  int i;

  switch (oldnode->type) {
  case LIST:
    {
      newnode = list_to_node(nlistalloc(),oldnode->nlist);
      for (i=0; i < oldnode->nlist; i++) 
        {
          newnode->list[i] = node_copy(oldnode->list[i]);
        }
      break;
    }
  case NUMBER:
    {
      newnode = double_to_node(oldnode->number);
      break;
    }
  case STRING: 
    {
      newnode = string_to_node(oldnode->string);
      break;
    }
  case SYMBOL:
    {
      newnode = symbol_to_node(oldnode->symbol);
      break;
    }
  case PROC:
    {
      newnode = 
        procedure_to_node(oldnode->proc->symbols,
                          oldnode->proc->nargs,
                          node_copy(oldnode->proc->body),
                          copy_environment_list(oldnode->proc->env));
      break;
    }
  case PAIR:
    {
      newnode = pair_to_node(node_copy(oldnode->pair->car), node_copy(oldnode->pair->cdr));
      break;
    }
  case NIL:
    {
      newnode = nil_alloc();
      break;
    }
  default:
    {
      newnode = oldnode;
      printf("remember to update node_copy for type %d\n", oldnode->type);
    }
  }
  return newnode;
}


// environments

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
}

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
    return newenv;
}

void
bind_in_current_env(struct environment **envlist, char *symbol, struct node *value)
{
    int i, j;

    // first look it up 
    struct variable *look = lookup(symbol, envlist);

    if (look != NULL) { 
        // if it already exists, just change the value pointer to the new one
        look->value = value;
    }
    else {
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

struct variable *
lookup(char *symbol, struct environment **envlist)
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
            if (strcmp(envlist[i]->vars[j]->symbol, symbol) == 0) {
                return envlist[i]->vars[j];
            }
        }
    }
    #ifdef DEBUG
    printf("lookup: looked up a nonexistent variable: %s\n", symbol);
    #endif
    return NULL;
}

struct node *
lookup_value(struct environment **envlist, struct node *expr)
{
    struct variable *var = lookup(expr->symbol, envlist);
    struct node *result;
    if (var != NULL)
        result = node_copy(var->value);
    else {
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


struct node *
eval(struct node *expr, struct environment **env)
{
    switch (expr->type) {
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
            if (primitive_proc(expr))
                return expr;
            else {
                return lookup_value(env, expr);
            }
            break;
        default:
            printf("eval: Unknown expression type %d;\n", expr->type);
    }
}

struct node *
eval_setcar(struct node *expr, struct environment **env)
{
    struct variable *var = lookup(expr->list[1]->symbol,env);

    var->value->pair->car = eval(expr->list[2], env);

    return nil_alloc();
}

struct node *
eval_setcdr(struct node *expr, struct environment **env)
{
    struct variable *var = lookup(expr->list[1]->symbol,env);

    var->value->pair->cdr = eval(expr->list[2], env);

    return nil_alloc();
}

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
    // create new environment with this bindings
    struct environment **newenv = copy_environment_list(env);
    extend_envlist(newenv, varlist, i);

    // create node to represent the expressions in the rest of the let
    struct node *body = list_to_node(nlistalloc(), expr->nlist - 1);

    // put begin node at the beginning of list
    body->list[0] = symbol_to_node("begin"); 

    // rest of list is the expressions from the let
    for (i = 1; i+1 < expr->nlist; i++) {
        body->list[i] = expr->list[i+1];
    }
    body->nlist = expr->nlist - 1;

    return eval(body, newenv);
}

struct node *
eval_cond(struct node *expr, struct environment **env)
{
    int i;
    for (i = 1; i < expr->nlist; i++) 
      {
        if ((expr->list[i]->list[0]->type == SYMBOL && !strcmp(expr->list[i]->list[0]->symbol,"else")) 
            || istrue(expr->list[i]->list[0], env)) 
          {
            return eval(expr->list[i]->list[1], env);
          }
      }
    return nil_alloc();
}

struct node *
eval_quote(struct node *expr, struct environment **env)
{
    return expr->list[1];
}

struct node *
eval_delay(struct node *expr, struct environment **env)
{
    return procedure_to_node(NULL, 0, expr->list[1], env);
}

struct node *
eval_lambda(struct node *expr, struct environment **env)
{
  char *arglist[expr->list[1]->nlist];
  int i;

  // collects the tokens from the list in the second position in another array
  for (i = 0; i < expr->list[1]->nlist; i++) 
    {
      arglist[i] = expr->list[1]->list[i]->symbol;
    }

  return procedure_to_node(arglist, i, expr->list[2], env);
}

struct node *
eval_define(struct node *expr, struct environment **env)
{
  struct node *varvalue;
  char *name;

  if (expr->list[1]->type == LIST) {
    name = expr->list[1]->list[0]->symbol;

    // arglist
    char *arglist[expr->list[1]->nlist - 1];
    int i,j;
    for (i = 0; i+1 < expr->list[1]->nlist; i++) {
      arglist[i] = expr->list[1]->list[i+1]->symbol;
    }

    // body
    // create node to represent the expressions in the rest of the define
    struct node *body = list_to_node(nlistalloc(), expr->nlist - 1);

    // put begin node at the beginning of list
    body->list[0] = symbol_to_node("begin"); 

    // rest of list is the expressions from the let
    for (j = 1; j+1 < expr->nlist; j++) {
      body->list[j] = expr->list[j+1];
    }
    
    // env
    // lexical
    /* struct environment **procenv = copy_environment_list(env); */

    /* varvalue = procedure_to_node(arglist, i, body, procenv); */

    /* bind_in_current_env(procenv, name, varvalue); */
    
    // dynamic
    varvalue = procedure_to_node(arglist, i, body, env);
  }
  else {
    name = expr->list[1]->symbol;
    varvalue = eval(expr->list[2], env);
  }
  bind_in_current_env(env, name, varvalue);
  return varvalue;
}

struct node *
eval_load(struct node *expr, struct environment **env)
{
  if (expr->list[1]->type == STRING)
    {
      char* filename = expr->list[1]->string;
      return eval(parse_file(filename), env);
    }
  return nil_alloc();
}

gboolean
istrue(struct node *expr, struct environment **env)
{
  struct node *val = eval(expr, env);

  if (val->type == NUMBER && val->number == 0)
    {
      return 0;
    }
  else
    {
      return 1;
    }
}

int
node_equal(struct node *n1, struct node *n2)
{
  if (n1->type == SYMBOL && n2->type == SYMBOL) 
    {
      if (!strcmp(n1->symbol,n2->symbol))
        return 1;
    }
  else if (n1->type == NUMBER && n2->type == NUMBER)
    {
      if (n1->number == n2->number)
        return 1;
    }
  else if (n1->type == NIL && n2->type == NIL) 
    {
      return 1;
    }
  return 0;
}

struct node *
eval_if(struct node *expr, struct environment **env)
{
  if (istrue(expr->list[1], env))
    {
      return eval(expr->list[2], env);
    }
  else
    {
      return eval(expr->list[3], env);
    }
}

struct node *
eval_application(struct node *expr, struct environment **env)
{
    int i;
    for (i = 0; i < expr->nlist; i++) 
      {
        expr->list[i] = eval(expr->list[i], env);
      }

    return apply(expr->list[0], (expr->list+1), (i-1));
}

struct node *
list_to_ll(struct node **nodelist, int n)
{
    struct node *topnode, *curnode, *nextnode;
    int i;
    
    if (n == 0) return nil_alloc();

    topnode = curnode = pair_to_node(nodelist[0], NULL);
    for (i = 1; i < n; i++) 
      {
        curnode->pair->cdr = pair_to_node(nodelist[i], NULL);
        curnode = curnode->pair->cdr;
      }
    curnode->pair->cdr = nil_alloc();
    return topnode;
}

struct node *
apply_compound(struct node *proc, struct node *args[], int n)
{
    int i, dot;
    struct variable *varlist[proc->proc->nargs];

    dot = 0;
    for (i = 0; i < proc->proc->nargs; i++) {
        if (proc->proc->symbols[i][0] == '.') {
            dot = 1;
            break;
        }
        varlist[i] = varalloc();
        varlist[i]->symbol = proc->proc->symbols[i];
        varlist[i]->value = args[i];
    }
    if (dot == 1) {
        varlist[i] = varalloc();
        varlist[i]->symbol = proc->proc->symbols[i+1];
        varlist[i]->value = list_to_ll(args+i, n-i);
        i++;
    }

    extend_envlist(proc->proc->env, varlist, i);

    return eval(proc->proc->body, proc->proc->env);
}

struct node *
apply(struct node *proc, struct node **args, int n)
{
  if (primitive_proc(proc)) 
    {
      return apply_prim(proc, args, n);
    }
  else 
    {
      return apply_compound(proc, args, n);
    }
}

struct node *
apply_prim(struct node *proc, struct node *args[], int n)
{
    int i;
    double total;
    struct node *result;

    if (proc->symbol[0] == '+') 
      {
        total = 0;
        for (i = 0; i < n; i++)
          {
            total += args[i]->number;
          }
        result = double_to_node(total);
      }
    else if (proc->symbol[0] == '-')
      {
        total = args[0]->number;
        for (i = 1; i < n; i++)
          {
            total = total - args[i]->number;
          }
        result = double_to_node(total);
      }
    else if (proc->symbol[0] == '*')
      {
        total = 1;
        for (i = 0; i < n; i++)
          {
            total = total * args[i]->number;
          }
        result = double_to_node(total);
      }
    else if (proc->symbol[0] == '/')
      {
        total = args[0]->number;
        for (i = 1; i < n; i++)
          {
            total = total / args[i]->number;
          }
        result = double_to_node(total);
      }
    else if (proc->symbol[0] == '=') 
      {
        total = 1;
        for (i = 0; i < n; i++)
          {
            total = total && (args[0]->number == args[i]->number);
          }
        result = bool_to_node(total);
      }
    else if (proc->symbol[0] == '<') 
      {
        result = bool_to_node((args[0]->number < args[1]->number));
      }
    else if (proc->symbol[0] == '>') 
      {
        result = bool_to_node((args[0]->number > args[1]->number));
      }
    else if (!strcmp(proc->symbol,"abs")) 
      {
        total = fabs(args[0]->number);
        result = double_to_node(total);
      }
    else if (!strcmp(proc->symbol,"cons")) 
      {
        result = pair_to_node(args[0],args[1]);
      }
    else if (!strcmp(proc->symbol,"car"))
      {
        result = args[0]->pair->car;
      }
    else if (!strcmp(proc->symbol,"cdr"))
      {
        result = args[0]->pair->cdr;
      }
    else if (!strcmp(proc->symbol,"null?")) 
      {
        result = bool_to_node((args[0]->type == NIL));
      }
    else if (!strcmp(proc->symbol,"number?"))
      {
        result = bool_to_node((args[0]->type == NUMBER));
      }
    else if (!strcmp(proc->symbol,"string?"))
      {
        result = bool_to_node((args[0]->type == STRING));
      }
    else if (!strcmp(proc->symbol,"symbol?")) 
      {
        result = bool_to_node((args[0]->type == SYMBOL));
      }
    else if (!strcmp(proc->symbol,"pair?"))
      {
        result = bool_to_node((args[0]->type == PAIR));
      }
    else if (!strcmp(proc->symbol,"read"))
      {
        result = read_list();
      }
    else if (!strcmp(proc->symbol,"eq?")) 
      {
        result = bool_to_node(node_equal(args[0], args[1]));
      }
    else if (!strcmp(proc->symbol,"display")) 
      {
        minimal_print_node(args[0]);
        result = nil_alloc();
      }
    else if (!strcmp(proc->symbol,"begin")) 
      {
        result = args[n-1];
      }
    else if (!strcmp(proc->symbol,"apply")) 
      {
        result = apply(args[0],(args+1),(n-1));
      }
    else if (!strcmp(proc->symbol,"listconv")) 
      {
        result = list_to_ll(args, n);
      }
    else
      {
        result = nil_alloc();
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

struct node *
read_list()
{
    int c;
    struct node *root;

    printf("!>> ");
    c = getch(); //remove newline that led us here
    if (c == '\n')
        ;
    else
        ungetch(c);
    while ((c = getch()) != '\n') {
        if (!iswspace(c)) {
            ungetch(c);
            root = parse_token_ll();
            //print_node(root);
        }
    } 
    ungetch('\n');
    return root;
}

main()
{
    int c;

    // the global environment
    struct environment **globalenv = envlistalloc();
    globalenv[0] = envalloc();
    globalenv[0]->vars = varlistalloc();

    struct node *nil = nil_alloc();
    // the symbol nil is manually bound to the value nil 
    bind_in_current_env(globalenv, "nil", nil);

    // get our defaults
    eval(parse_file("defaults.scm"), globalenv);
    /* eval(parse_file("example.scm"), globalenv); */

    printf(">>");
    while ((c = getch()) != EOF) {
        if (!iswspace(c)) {
            ungetch(c);

            /* // parse our input */
            /* // and get the root of the resulting s expression */
            /* struct node *root = parse(); */

            /* // print the unevaled s-exp */
            /* //print_node(root); */

            /* // eval the s expression */
            /* struct node *result = eval(root, globalenv); */

            /* // print the resulting s expression */
            /* print_node(result); */
            
            // do it all
            print_node(eval(parse(), globalenv));

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
