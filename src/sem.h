#ifndef SEM_H
#define SEM_H

#include "tree.h"
#include "table.h"

/* Renvoie l'indice de la fonction dans la table globale */
int i_fonct(char *nom, Tables *tables);

/* Renvoie l'indice de la table correspondant au nom de la fonction */
int i_table_vers_i_symb(int i_table, Tables *tables);

/* Vérifie les déclarations des fonctions et variables */
int verif_decl(int i_table, Tables *tables, Node *node);

/* Vérifie les types des fonctions et variables dans les expressions */
int verif_type(int i_table, Tables *tables, Node *node);

/* Vérifie si le main est bien déclaré */
int verif_main(TableSymb global);


#endif