int getch(void);
void ungetch(int);

struct node *
parse();

struct node *
parse_ll();

struct node *
parse_file(char *filename);
