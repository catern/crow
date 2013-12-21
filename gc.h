typedef void (*freefunc)(void *);

void *
malloc_mon(size_t, freefunc);

GString *
stralloc();

void
garbage_collect(struct environment **);
