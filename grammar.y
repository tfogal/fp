%code requires {
#include <inttypes.h>
#include <stdio.h>

#define YYPARSE_PARAM scanner
#define YYLEX_PARAM   scanner
}
%code requires {
#include "parsefwd.h"
}
%code{
#include "lexer.h"
}
%parse-param { yyscan_t scanner }
%lex-param { yyscan_t scanner }
%locations
%define api.pure full

%union {
  int64_t vi;
  double vf;
  char* vs;
}

%token <vi> INTEGER
%token <vf> FLOAT
%token <vs> STRING

%%

input
  : /* nothing */
  | INTEGER input { printf("int: %ld\n", $1); }
  | FLOAT input { printf("float: %lf\n", $1); }
  | STRING input { printf("int: %s\n", $1); }
  ;
