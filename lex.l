%option header-file="tokens.h"
%option nounput
%option noyyalloc noyyfree noyyrealloc
%option reentrant
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

%%

/* redefining this ourselves so we can get around an 'unused var' warning. */
void*
yyalloc(yy_size_t size, yyscan_t scanner)
{
  (void)scanner;
  return (void*)malloc(size);
}

/* redefining this ourselves so we can get around an 'unused var' warning. */
void*
yyrealloc(void* ptr, yy_size_t size, yyscan_t scanner)
{
  (void)scanner;
  /* The cast to (char *) in the following accommodates both
   * implementations that use char* generic pointers, and those
   * that use void* generic pointers.  It works with the latter
   * because both ANSI C and C++ allow castless assignment from
   * any pointer type to void*, and deal with argument conversions
   * as though doing an assignment.
   */
  return (void*)realloc((char*)ptr, size);
}

/* redefining this ourselves so we can get around an 'unused var' warning. */
void
yyfree(void* ptr, yyscan_t scanner)
{
  (void)scanner;
  free((char*)ptr); /* see yyrealloc() for (char *) cast */
}
