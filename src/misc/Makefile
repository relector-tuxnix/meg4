#there's one ULL constant in mbedtls' genprime, that's why it can't be ANSI C...
CFLAGS = -std=c99 -pedantic -Wall -Wextra

all: lictool

lictool: lictool.c ../editors/pro.c
	$(CC) $(CFLAGS) lictool.c -o lictool

langchk: langchk.c
	@$(CC) $(CFLAGS) langchk.c -o langchk
	@./langchk
	@rm langchk

clean:
	@rm lictool langchk 2>/dev/null || true
