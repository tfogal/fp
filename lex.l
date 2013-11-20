%option header-file="tokens.h"
%option nounput
%top{
#define _POSIX_C_SOURCE 200809L
#define YY_NO_INPUT
#include <stdio.h>
}

CHAR ([[:alpha:]]|\-)
IDENTIFIER [[:alpha:]]+[[:alnum:]]*
OPTION \-+[[:alnum:]]+

%%
\| { printf("|"); }
whoami { printf("%s", getlogin()); }
{IDENTIFIER} { printf("id: '%s'", yytext); }
{OPTION} { printf("opt: '%s'", yytext); }
. { printf(" "); }
