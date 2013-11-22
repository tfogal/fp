#include <stdlib.h>
#include "lexer.h"
#include "tokens.h"

int
main(int argc, char* const argv[])
{
  yyscan_t scan;
  yylex_init(&scan);
  yyset_in(stdin, scan);
  if(argc == 2) {
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL) {
      fprintf(stderr, "Could not open '%s'\n", argv[1]);
      return EXIT_FAILURE;
    }
    yyset_in(fp, scan);
  }
  if(0) {
    YYSTYPE lval;
    YYLTYPE lloc;
    printf("yylex: %d\n", yylex(&lval, &lloc, scan));
  } else {
    yyparse(scan);
  }
  yylex_destroy(scan);
  return EXIT_SUCCESS;
}
