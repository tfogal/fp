WARN:=-Wall -Wextra
CFLAGS:=-std=gnu99 -D_REENTRANT $(WARN) -ggdb3
LDFLAGS=-lfl
src:=main.c lex.c
obj:=$(src:.c=.o)

all: $(obj) interp

interp: main.o lex.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(obj) interp
	rm -f lex.c tokens.h

main.c: tokens.h
tokens.h: lex.c
