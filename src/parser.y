%{
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "../src/tree.h"
#include "parser.tab.h"
int yylex();
void yyerror(Node ** arbre_abstrait, char * msg);
extern int num_ligne;
extern int num_car;
extern Node ** arbre_abstrait;
%}

%union {
	Node *node;
	char byte;
	int num;
	char ident[64];
	int type;
	char comp[3];
}

%token <ident> IDENT
%token <type> TYPE
%token IF ELSE WHILE RETURN
%token <num> NUM
%token <byte> CHARACTER ADDSUB DIVSTAR
%token <comp> EQ ORDER 
%token OR AND
%token <ident> VOID

%type <node> Prog DeclVars Declarateurs DeclFoncts DeclFonct EnTeteFonct Parametres ListTypVar Corps SuiteInstr Instr Exp TB FB M E T F LValue Arguments ListExp

%expect 1

%parse-param { Node ** arbre_abstrait }

%%
Prog:  DeclVars DeclFoncts              	{ $$ = makeNode(program); 
		                                  addChild($$, $1);
		                                  addSibling($1, $2);
		                                  *arbre_abstrait = $$; }
    ;
DeclVars:
       DeclVars TYPE Declarateurs ';'   	{ Node *t;
						  $$ = $1;
						  t=makeNode(type);
						  t->type = $2;
						  addChild($$, t);
						  addChild(t, $3); }
    | 						{ $$ = makeNode(declarations); }
    ;
Declarateurs:
       Declarateurs ',' IDENT 			{ $$ = $1;
						  Node *t=makeNode(ident);
						  strcpy(t->ident, $3);
						  addSibling($$, t); }
    |  Declarateurs ',' IDENT '[' NUM ']'	{ $$ = $1;
						  Node *t = makeNode(tableau);
						  Node *t1=makeNode(ident);
						  strcpy(t1->ident, $3);
						  addSibling($$, t); 
						  addChild(t, t1);
						  Node *t2 = makeNode(num); t2->num = $5; addChild(t, t2); }
    |  IDENT					{ $$ = makeNode(ident); strcpy($$->ident, $1); }
    |  IDENT '[' NUM ']'			{ $$ = makeNode(tableau); 
						  Node *t1 = makeNode(ident); strcpy(t1->ident, $1); addChild($$, t1);
						  Node *t2 = makeNode(num); t2->num = $3; addChild($$, t2); }
    ;
DeclFoncts:
       DeclFoncts DeclFonct			{ $$ = $1;
						  addChild($$, $2);}
    |  DeclFonct				{ $$ = makeNode(fonctions);
                                          	  addChild($$, $1);}
    ;
DeclFonct:
       EnTeteFonct Corps			{ $$ = $1;
						  addChild($$, $2); }
    ;
EnTeteFonct:
       TYPE IDENT '(' Parametres ')' 		{ $$ = makeNode(fonction);
						  Node *t1 = makeNode(type);
						  t1->type = $1;
						  Node *t2 = makeNode(ident);
						  strcpy(t2->ident, $2);
						  addChild($$, t1);
						  addChild($$, t2); 
						  addChild($$, $4); }
    |  VOID IDENT '(' Parametres ')'    	{ $$ = makeNode(fonction);
						  Node *t1 = makeNode(type);
						  t1->type = 0;
						  Node *t2 = makeNode(ident);
						  strcpy(t2->ident, $2);
						  addChild($$, t1);
						  addChild($$, t2); 
						  addChild($$, $4); }
    ;
Parametres:
       VOID					{ $$ = makeNode(parametres); 
						  Node *t = makeNode(type); 
						  t->type = 0;
						  addChild($$, t); }
    |  ListTypVar				{ $$ = makeNode(parametres); 
					  	  addChild($$, $1); }
    ;
ListTypVar:
       ListTypVar ',' TYPE IDENT		{ $$ = $1;
						  Node *t1 = makeNode(type); 
						  t1->type = $3;
						  Node *t2 =makeNode(ident);
						  strcpy(t2->ident, $4);
						  addSibling($$, t1);
						  addChild(t1, t2); }
    |  ListTypVar ',' TYPE IDENT '[' ']'	{ $$ = $1;
						  Node *t = makeNode(tableau); 
						  Node *t1 = makeNode(type); 
						  t1->type = $3;
						  Node *t2 =makeNode(ident);
						  strcpy(t2->ident, $4);
						  addSibling($$, t);
						  addChild(t, t1); 
						  addChild(t, t2); }
    |  TYPE IDENT				{ $$ = makeNode(type); 
						  $$->type = $1;
						  Node *t = makeNode(ident); 
						  strcpy(t->ident, $2);
						  addChild($$, t); }
    |  TYPE IDENT '[' ']'			{ $$ = makeNode(tableau); 
						  Node *t1 = makeNode(type); t1->type = $1; addChild($$, t1);
						  Node *t2 = makeNode(ident); strcpy(t2->ident, $2); addChild($$, t2); }
    ;
Corps: '{' DeclVars SuiteInstr '}' 		{ $$ = $2;
						  addSibling($$, $3); }
    ;
SuiteInstr:
       SuiteInstr Instr				{ $$ = $1;
						  addChild($$, $2); }
    |						{ $$ = makeNode(instructions); }
    ;
Instr:
       LValue '=' Exp ';'			{ $$ = makeNode(assign);
						  addChild($$, $1);
						  addChild($$, $3); }
    |  IF '(' Exp ')' Instr			{ $$ = makeNode(si);
						  addChild($$, $3);
						  addChild($$, $5); }
    |  IF '(' Exp ')' Instr ELSE Instr		{ $$ = makeNode(si);
						  addChild($$, $3);
						  addChild($$, $5); 
						  addChild($$, $7); }
    |  WHILE '(' Exp ')' Instr			{ $$ = makeNode(tantque);
						  addChild($$, $3);
						  addChild($$, $5); }
    |  IDENT '(' Arguments  ')' ';'		{ $$ = makeNode(fonction);
						  Node *t = makeNode(ident);
						  strcpy(t->ident, $1);
						  addChild($$, t);
						  if ($3)
						  	addChild($$, $3); }
    |  RETURN Exp ';'				{ $$ = makeNode(retour); 
						   addChild($$, $2); }
    |  RETURN ';'				{ $$ = makeNode(retour); }
    |  '{' SuiteInstr '}'			{ $$ = $2; }
    |  ';'					{ ; }
    ;
Exp :  Exp OR TB				{ $$ = makeNode(ou);
						  addChild($$, $1);
						  addChild($$, $3); }
    |  TB					{ $$ = $1; }
    ;
TB  :  TB AND FB				{ $$ = makeNode(et);
						  addChild($$, $1);
						  addChild($$, $3); }
    |  FB					{ $$ = $1; }
    ;
    ;
FB  :  FB EQ M					{ $$ = makeNode(eq);
						  strcpy($$->comp, $2);
						  addChild($$, $1);
						  addChild($$, $3); }
    |  M					{ $$ = $1; }
    ;	
M   :  M ORDER E				{ $$ = makeNode(ordre);
						  strcpy($$->comp, $2);
						  addChild($$, $1);
						  addChild($$, $3); }
    |  E					{ $$ = $1; }
    ;
E   :  E ADDSUB T				{ $$ = makeNode(addsub);
						  $$->byte = $2;
						  addChild($$, $1);
						  addChild($$, $3); }
    |  T					{ $$ = $1; }
    ;    
T   :  T DIVSTAR F 				{ $$ = makeNode(divstar);
						  $$->byte = $2;
						  addChild($$, $1);
						  addChild($$, $3); }
    |  F					{ $$ = $1; }
    ;
F   :  ADDSUB F					{ $$ = makeNode(addsub); 
						  $$->byte = $1;
						  addChild($$, $2); }
    |  '!' F					{ $$ = makeNode(non); 
						  addChild($$, $2); }
    |  '(' Exp ')'				{ $$ = $2; }
    |  NUM					{ $$ = makeNode(num); $$->num = $1;}
    |  CHARACTER				{ $$ = makeNode(charact); $$->byte = $1; }
    |  LValue					{ $$ = $1; }
    |  IDENT '(' Arguments  ')'			{ $$ = makeNode(fonction);
						  Node *t = makeNode(ident);
						  strcpy(t->ident, $1);
						  addChild($$, t);
						  if ($3)
						  	addChild($$, $3); }
    ;	
LValue:
       IDENT					{ $$ = makeNode(ident);
						  strcpy($$->ident, $1); }
    |  IDENT '[' Exp ']'			{ $$ = makeNode(tableau);
						  Node *t = makeNode(ident);
						  strcpy(t->ident, $1); 
						  addChild($$, t); addChild($$, $3); }
    ;
Arguments:
       ListExp					{ $$ = $1; }
    |						{ $$ = NULL; }
    ;
ListExp:	
       ListExp ',' Exp				{ $$ = $1;
						  addSibling($$, $3); }
    |  Exp					{ $$ = $1; }
    ;
%%

void yyerror(Node ** arbre_abstrait, char * msg) {
    fprintf(stderr, "%s : L:%d C:%d\n", msg, num_ligne, num_car);
}

