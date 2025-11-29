CC=gcc -g


build: gc.o runtime.o test.o
	$(CC) gc.o runtime.o test.o -o a.out

gc.o: gc.c gc.h
	$(CC) -c gc.c

runtime.o: runtime.c gc.h globals.h vm_types.h runtime.h
	$(CC) -c runtime.c

test.o: test.c gc.h runtime.h globals.h
	$(CC) -c test.c


clean:
	rm -f ./*.o
	rm -f ./a.out	