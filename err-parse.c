#include <stdio.h>
#include "parsefwd.h"

void
yyerror(YYLTYPE* loc, yyscan_t scanner, const char* msg)
{
  (void) loc;
  (void) scanner;
  fprintf(stderr, "Parse error: %s\n", msg);
}
