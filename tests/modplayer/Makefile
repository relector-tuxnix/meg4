CFLAGS=-ansi -pedantic -Wall -Wextra -g
ifneq ($(TEST_PA),)
CFLAGS+=-DTEST_PA=1
LIBS=-lportaudio
else
LIBS=-lSDL2
endif

all: modplayer

dsp.o: ../../src/dsp.c
	gcc $(CFLAGS) ../../src/dsp.c -c -o dsp.o

main.o: main.c
	gcc $(CFLAGS) main.c -c -o main.o

modplayer: main.o dsp.o
	gcc main.o dsp.o -o modplayer $(LIBS) -lm

clean:
	rm *.o modplayer 2>/dev/null || true

