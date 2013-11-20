#include <stdlib.h>
#include "tokens.h"

int
main(int argc, char* const argv[])
{
  yyin = stdin;
  if(argc == 2) {
    yyin = fopen(argv[1], "r");
    if(yyin == NULL) {
      fprintf(stderr, "Could not open '%s'\n", argv[1]);
      return EXIT_FAILURE;
    }
  }
  printf("yylex: %d\n", yylex());
  return EXIT_SUCCESS;
}
