#include <stdio.h>
#include "types.h"
#include "printnode.h"

void
print_list(struct node *expr)
{
    if (expr->nlist == 0) {
        printf("()");
        return;
    }

    int i;
    printf("(");
    for (i = 0; i < expr->nlist; i++) {
        minimal_print_node(expr->list[i]);
        printf(" ");
    }
    // remove ending space in last token in the list with \b, backspace
    printf("\b)");
}

void
minimal_print_node(struct node *expr)
{
    switch (expr->type) {
    case NUMBER: {
        printf("%f", expr->number);
        break;
    }
    case SYMBOL: {
        printf("%s", expr->symbol);
        break;
    }
    case PROC: {
        printf("<procedure>");
        break;
    }
    case LIST: {
        print_list(expr);
        break;
    }
    case PAIR: {
        printf("<pair>");
        break;
    }
    case STRING: {
        printf("%s", expr->string->str);
        break;
    }
    default: {
        printf("<type: %d>", expr->type);
    }
    }
}

void
print_node(struct node *expr)
{
    int i;
    // colorful and verbose printing of values!
    // color codes are really hideous, TODO find a library for this

    switch (expr->type) {
    case NUMBER: {
        printf("\033[1;37mType:\033[0m Number\n");

        printf("\033[1;37mValue:\033[0m %f\n", expr->number);
        break;
    }
    case SYMBOL: {
        printf("\033[1;37mType:\033[0m Symbol\n");

        printf("\033[1;37mValue:\033[0m %s\n", expr->symbol);
        break;
    }
    case PROC: {
        printf("\033[1;37mType:\033[0m Procedure\n");

        printf("\033[1;37mArgs:\033[0m ");
        for (i = 0; i < expr->proc->nargs; i++) {
            printf("%s ", expr->proc->symbols[i]);
        }

        printf("\n\033[1;37mBody:\033[0m \n");
        minimal_print_node(expr->proc->body);

        printf("\n");
        break;
    }
    case LIST: {
        printf("\033[1;37mType:\033[0m List\n");

        printf("\033[1;37mValue:\033[0m ");
        print_list(expr);

        printf("\n");
        break;
    }
    case STRING: {
        printf("\033[1;37mType:\033[0m String\n");

        printf("\033[1;37mValue:\033[0m ");
        printf("%s\n", expr->string->str);
        break;
    }
    case PAIR: {
        printf("\033[1;37mType:\033[0m Pair\n");

        printf("\033[1;37mCar:\033[0m ");
        minimal_print_node(expr->pair->car);

        printf("\n\033[1;37mCdr:\033[0m ");
        minimal_print_node(expr->pair->cdr);

        printf("\n");
        break;
    }
    default: {
        printf("\033[1;37mType:\033[0m %d\n", expr->type);
    }
    }
}
