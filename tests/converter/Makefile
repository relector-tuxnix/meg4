CFLAGS=-ansi -pedantic -Wall -Wextra -g -I../../src/editors

all: ../../src/data.o converter

../../src/data.o:
	make -C ../../src data.o

formats.o: ../../src/editors/formats.c
	gcc $(CFLAGS) -Wno-long-long ../../src/editors/formats.c -c -o formats.o

zip.o: ../../src/editors/zip.c
	gcc $(CFLAGS) ../../src/editors/zip.c -c -o zip.o

dsp.o: ../../src/dsp.c
	gcc $(CFLAGS) ../../src/dsp.c -c -o dsp.o

main.o: main.c
	gcc $(CFLAGS) main.c -c -o main.o

converter: main.o formats.o zip.o dsp.o
	gcc main.o formats.o zip.o dsp.o ../../src/data.o -o converter -lm

clean:
	rm *.o converter *.zip 2>/dev/null || true

