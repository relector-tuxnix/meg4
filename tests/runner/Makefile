CFLAGS = -ansi -pedantic -Wall -Wextra -Wno-pragmas -I../../src -g -DDEBUG=1

all: libmeg4 runner

libmeg4:
	@make -C ../../src all DEBUG=1 NOLUA=$(NOLUA) NOEDITORS=$(NOEDITORS)

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

runner: ../../src/libmeg4.a main.o
	$(CC) $(LDFLAGS) main.o ../../src/libmeg4.a -o runner -lm

clean:
	@rm runner *.o 2>/dev/null || true

distclean: clean
	@make --no-print-directory -C ../../src distclean 2>/dev/null || true
