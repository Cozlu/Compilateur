#ifndef TABLE_H
#define TABLE_H

#include <stdio.h>
#include "tree.h"

typedef struct {
   size_t adr;
   int type;
   int taille;
   union { // 0 non, 1 oui
        int param;
        int fonct;
   } info;
   char ident[64]; 
} Symbole;

typedef struct {
    int nb_symb;
    int taille_max;
    size_t deplct;
    Symbole * table;
} TableSymb;

typedef struct {
    int nb_tab;
    int taille_max;
    TableSymb * tables;
} Tables;

/* Construit les tables de symboles à partir de l'abre abstrait */
int construit_tables_symb(Tables *tables, Node *arbre_abstrait);

/* Affiche les tables de symboles */
void aff_tables(Tables tables);

#endif