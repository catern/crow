typedef void (*freefunc)(void *);

void *
malloc_mon(size_t, freefunc);

void
garbage_collect(struct environment **);
