default:
	gcc $(shell pkg-config --cflags --libs glib-2.0) -DDEBUG -ggdb gc.c types.c parser.c printnode.c main.c

production:
	gcc $(shell pkg-config --cflags --libs glib-2.0) gc.c types.c parser.c printnode.c main.c

tags: 
	rm -f TAGS && find . -name "*.[ch]" -print | xargs etags -a
