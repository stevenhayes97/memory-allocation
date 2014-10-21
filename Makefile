all: libmem1.so libmem2.so libmem3.so 1testbit 2testbit test1

1testbit: testbit.c libmem1.so
	gcc -lmem1 -L. -o 1testbit testbit.c -Wall -Werror -g

2testbit: testbit.c libmem2.so
	gcc -lmem2 -L. -o 2testbit testbit.c -Wall -Werror -g

test1: tester.c libmem1.so
	gcc -lmem1 -L. -o test tester.c -Wall -Werror -g


libmem1.so: mem_bitmap.o
	gcc -shared -o libmem1.so mem_bitmap.o -g

libmem3.so: mem.o
	gcc -shared -o libmem3.so mem.o -g

libmem2.so: mem_bitmap_wl2.o
	gcc -shared -o libmem2.so mem_bitmap_wl2.o -g

#libmem1.so: mem.o
#	gcc -shared -o libmem1.so mem.o -g

mem_bitmap_wl2.o: mem_bitmap_wl2.c
	gcc -c -fpic mem_bitmap_wl2.c -Wall -Werror -g

mem_bitmap.o: mem_bitmap.c
	gcc -c -fpic mem_bitmap.c -Wall -Werror -g

mem.o: mem.c
	gcc -c -fpic mem.c -Wall -Werror -g
clean:
	rm -f libmem1.so libmem2.so libmem3.so mem.o mem_bitmap.o mem_bitmap_wl2.o test testmain testbit
