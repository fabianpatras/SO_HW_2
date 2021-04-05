CC=gcc
FLAGS=-g -Wall -Wextra -I util/

OBJ=so_stdio.o utils.o

build: libso_stdio.so

libso_stdio.so: $(OBJ)
	$(CC) -shared $^ -o $@

%.o: src/%.c
	$(CC) $(FLAGS) -c $^ -o $@

test:
	cp libso_stdio.so checker-lin/
	cd checker-lin/ && make -f Makefile.checker

main: $(OBJ) test_main.c
	$(CC) $(FLAGS) $^ -o $@

clean:
	rm -rf *.o libso_stdio.so
	rm -f checker-lin/libso_stdio.so
	rm -f main_test main