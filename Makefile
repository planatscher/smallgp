all:
	gcc smallgp.c -lm -o smallgp 
ansi:
	gcc smallgp.c -lm -ansi -pedantic -o smallgp 
debug:
	gcc smallgp.c -lm -g -o smallgp 
doc:
	a2ps smallgpdoc.txt -o smallgpdoc.ps --header="SmallGP - Documentation" 	
	gv smallgpdoc.ps &
clean:
	rm smallgp smallgpdoc.pdf
