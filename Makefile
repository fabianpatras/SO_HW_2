CC=gcc
FLAGS=-g -Wall -Wextra -I util/ -c

OBJ=so_stdio.o utils.o

build: libso_stdio.so

libso_stdio.so: $(OBJ)
	$(CC) -shared $^ -o $@

%.o: src/%.c
	$(CC) $(FLAGS) $^ -o $@

test:
	cp libso_stdio.so checker-lin/
	cd checker-lin/ && make -f Makefile.checker


clean:
	rm -rf *.o libso_stdio.so
	rm -f checker-lin/libso_stdio.so