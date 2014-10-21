all: libmem1.so libmem2.so libmem3.so testbit test1

testbit: testbit.c libmem1.so
	gcc -lmem1 -L. -o testbit testbit.c -Wall -Werror -g

test1: tester.c libmem1.so
	gcc -lmem1 -L. -o test tester.c -Wall -Werror -g


libmem1.so: mem_bitmap.o
	gcc -shared -o libmem1.so mem_bitmap.o -g

libmem3.so: mem.o
	gcc -shared -o libmem3.so mem.o -g

libmem2.so: mem.o
	gcc -shared -o libmem2.so mem.o -g

#libmem1.so: mem.o
#	gcc -shared -o libmem1.so mem.o -g

mem_bitmap.o: mem_bitmap.c
	gcc -c -fpic mem_bitmap.c -Wall -Werror -g

mem.o: mem.c
	gcc -c -fpic mem.c -Wall -Werror -g
clean:
	rm -f libmem1.so libmem2.so libmem3.so libmem4.so  mem.o mem_bitmap.o test testmain testbit
