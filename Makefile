all: libmem1.so libmem2.so libmem3.so

test1: tester.c libmem1.so
	gcc -lmem1 -L. -o test tester.c -Wall -Werror -g

libmem3.so: mem.o
	gcc -shared -o libmem3.so mem.o -g

libmem2.so: mem.o
	gcc -shared -o libmem2.so mem.o -g

libmem1.so: mem.o
	gcc -shared -o libmem1.so mem.o -g

mem.o: mem.c
	gcc -c -fpic mem.c -Wall -Werror -g
clean:
	rm -f libmem1.so mem.o test testmain
