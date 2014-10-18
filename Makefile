
test1: tester.c libmem1.so
	gcc -lmem1 -L. -o test tester.c -Wall -Werror -g

libmem1.so: mem.o
	gcc -shared -o libmem1.so mem.o

mem.o: mem.c
	gcc -c -fpic mem.c -Wall -Werror
clean:
	rm -f libmem1.so mem.o test
