#ifndef PARSEFWD_H
#define PARSEFWD_H

struct YYLTYPE; typedef struct YYLTYPE YYLTYPE;

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

extern void yyerror(YYLTYPE*, yyscan_t, const char*);

#endif
