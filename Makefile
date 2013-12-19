default:
	gcc gc.c types.c printnode.c main.c

tags: 
	rm -f TAGS && find . -name "*.[ch]" -print | xargs etags -a
