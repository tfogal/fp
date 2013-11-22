WARN:=-Wall -Wextra
CFLAGS:=-std=gnu99 -D_REENTRANT $(WARN) -ggdb3
YFLAGS:=--defines=tokens.h
LDFLAGS=-lfl
#src:=main.c lex.c grammar.c err-parse.c
obj:=main.o lex.o grammar.o err-parse.o

all: $(obj) interp

interp: main.o lex.o grammar.o err-parse.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(obj) interp
	rm -f lex.c tokens.h grammar.c lexer.h

main.c: lexer.h tokens.h
tokens.h: grammar.c
lexer.h: lex.c
