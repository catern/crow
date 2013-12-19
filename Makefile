default:
	gcc $(shell pkg-config --cflags --libs glib-2.0) gc.c types.c printnode.c main.c

tags: 
	rm -f TAGS && find . -name "*.[ch]" -print | xargs etags -a
