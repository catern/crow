default:
	gcc -DDEBUG -ggdb gc.c types.c parser.c printnode.c main.c

tags: 
	rm -f TAGS && find . -name "*.[ch]" -print | xargs etags -a
