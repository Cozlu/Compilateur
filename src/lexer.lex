%{
#include "../src/tree.h"
#include "parser.tab.h"
#include <string.h>

int num_ligne = 1;
int num_car = 1;
%}
%option nounput
%option noinput
%x COM
%%
"/*" num_car += yyleng; BEGIN COM;
<COM>. num_car += yyleng;
<COM>\n num_ligne++; num_car = 1;
<COM>"*/" num_car += yyleng; BEGIN INITIAL;
\/\/.* ;
void num_car += yyleng; yylval.num = 0; return VOID;
int|char num_car += yyleng; yylval.num = strcmp(yytext, "int") ? 1 : 2; return TYPE;
if num_car += yyleng; return IF;
else num_car += yyleng; return ELSE;
while num_car += yyleng; return WHILE;
return num_car += yyleng; return RETURN;
"'"\\n"'" yylval.byte = '\n'; return CHARACTER;
"'"\\t"'" yylval.byte = '\t'; return CHARACTER;
"'"\\\'"'" yylval.byte = '\''; return CHARACTER;
"'"([[:alnum:]]|[[:punct:]]|[ ])"'" num_car += yyleng; yylval.byte = yytext[1]; return CHARACTER;
=|,|\;|\(|\)|\{|\}|!|\[|\] num_car += yyleng; return yytext[0];
[0-9]+ num_car += yyleng; yylval.num = atoi(yytext); return NUM;
[-+] num_car += yyleng; yylval.byte = yytext[0]; return ADDSUB;
[*/%] num_car += yyleng; yylval.byte = yytext[0]; return DIVSTAR;
==|!= num_car += yyleng; strcpy(yylval.comp, yytext); return EQ;
"<"|"<="|">"|">=" num_car += yyleng; strcpy(yylval.comp, yytext); return ORDER;
"||" num_car += yyleng; return OR;
"&&" num_car += yyleng; return AND;
[a-zA-Z_][a-zA-Z_0-9]* num_car += yyleng; strcpy(yylval.ident, yytext); return IDENT;
\n num_ligne++; num_car = 1;
[ \t]+ num_car += yyleng;
. return 1;
%%
