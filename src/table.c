#include "table.h"
#include <stdlib.h>
#include <string.h>

/* Initialise la structure de stockage des tables */
static int init_tables(Tables * tables) {
    tables->nb_tab = 0;
    tables->taille_max = 10;
    if (!(tables->tables = (TableSymb*)malloc(sizeof(TableSymb) * 10)))
        return 1;
    return 0;
}

/* Initialise une table de symboles */
static int init_table_symb(TableSymb * table) {
    table->nb_symb = 0;
    table->taille_max = 10;
    table->deplct = 0;
    if (!(table->table = (Symbole*)malloc(sizeof(Symbole) * 10)))
        return 1;
    return 0;
}

/* Ajoute un symbole (type, identificateur et nombre d'éléments si c'est un tableau) à une table */
static int ajout_symb(TableSymb *table, int type, char *ident, int nb_elem, int est_param, int est_fonct, int local) {
    int i;
    for(i = 0; i < table->nb_symb; i++)
        if (!strcmp(ident, table->table[i].ident))
            return 2; // variable déjà définie
    if (table->nb_symb >= table->taille_max) {
        table->taille_max *= 2;
        if (!(table->table == (Symbole*)realloc(table->table, sizeof(Symbole) * table->taille_max)))
            return 3;
    }
    i = table->nb_symb;
    table->table[i].type = type;
    strcpy(table->table[i].ident, ident);
    if (local) { //adresse négative par rapport à rbp
        table->table[i].adr = -8*(nb_elem ? nb_elem : 1) - table->deplct;
    } else
        table->table[i].adr = table->deplct;
    table->table[i].taille = nb_elem;
    if (est_fonct)
        table->table[i].info.fonct = est_fonct;
    else
        table->table[i].info.param = est_param;
    table->deplct += 8*(nb_elem ? nb_elem : 1);
    table->nb_symb++;
    return 0;
}

/* Ajoute une table à la structure de stockage des tables */
static int ajout_table(Tables *tables, TableSymb table) {
    int i = tables->nb_tab;
    if (tables->nb_tab >= tables->taille_max) {
        tables->taille_max *= 2;
        if (!(tables->tables == (TableSymb*)realloc(tables->tables, sizeof(TableSymb) * tables->taille_max)))
            return 1;
    }
    tables->tables[i] = table;
    tables->nb_tab++;
    return 0;
}

/* Construit le symbole en déterminant son type et son identificateur à partir de l'arbre abstrait */
static int construit_symb(TableSymb *table, int type, Node *node, int est_param, int local) {
    int nb_elem;
    char *ident;
    if (!node) return 0;
    if (node->label == tableau) {
        if (est_param)
            ident = node->firstChild->nextSibling->ident;
        else
            ident = node->firstChild->ident;
        if (type <= 2) // ajustement du type vers un type de tableau (3 = array char, 4 = array int)
            type += 2;
        nb_elem = node->firstChild->nextSibling->num;
        if (nb_elem <= 0 && !est_param) {
            printf("semantic error : L:%d : %s est de taille négative\n", node->lineno, ident);
            return 2;
        }
    } else {
        if (type > 2) // ajustement du type vers un type de base (1 = char, 2 = int)
            type -= 2;
        ident = node->ident;
        nb_elem = 0;
    }
    switch(ajout_symb(table, type, ident, nb_elem, est_param, 0, local)) {
        case 2: 
            printf("semantic error : L:%d : %s est déjà déclaré\n", node->lineno, ident);
            return 2;
        case 3: return 3; 
    }
    if (!est_param)
        return construit_symb(table, type, node->nextSibling, est_param, local);
    return 0;
}

/* Construit une table de symboles */
static int construit_tab(TableSymb *table, Node *node, int est_param, int local) {
    int type;
    Node *noeud;
    for (Node *decl = node->firstChild; decl != NULL; decl = decl->nextSibling) { // parcours des variables
        if (decl->label == tableau && est_param){
            type = decl->firstChild->type;
            noeud = decl;
        } else {
            type = decl->type;
            noeud = decl->firstChild;
        }
        if (type == 0) continue; // void
        switch(construit_symb(table, type, noeud, est_param, local)) {
            case 2: return 2;
            case 3: return 3;
        }
    }
    return 0;
}

/* Vérifie si l'indetificateur correspond à un nom de fonction d'entrée sortie */
static int est_fct_IO(char *nom) {
    char *noms[4] = {"getchar", "getint", "putint", "putchar"};
    for (int i = 0; i < 4; i++)
        if (!strcmp(noms[i], nom))
            return 1;
    return 0;
}

/* Ajoute une fonction à la table des symboles globaux */
int ajout_fonct(TableSymb *table, Node *node) {
    char *ident;
    for (Node *fonct = node->firstChild; fonct != NULL; fonct = fonct->nextSibling) {
        ident = fonct->firstChild->nextSibling->ident;
        if (est_fct_IO(ident)) {
            printf("semantic error : L:%d : %s ne peut pas être redéfinie\n", node->lineno, ident);
            return 2;
        }
        switch(ajout_symb(table, fonct->firstChild->type, ident, 0, 0, 1, 0)) {
            case 2: 
                printf("semantic error : L:%d : %s est déjà déclaré\n", node->lineno, ident);
                return 2;
            case 3: return 3; 
        }
    }
    return 0;
}

/* Construit la table des symboles globaux à partir de l'arbre abstrait */
static int contruit_tab_var_globales(Tables *tables, Node *node) {
    TableSymb table;
    if (!tables) return 0;
    if (init_table_symb(&table)) 
        return 3;
    switch (construit_tab(&table, node, 0, 0)) {
        case 2: return 2;
        case 3: return 3;
    }
    if (ajout_table(tables, table)) 
        return 3;
    return 0;
}

/* Ajoute à la table globale les fonctions du programme tpc */
static int construit_tab_fonct(Tables *tables, Node *node) {
    Node *corps, *param;
    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) { // parcours des fonctions
        TableSymb table;
        param = child->firstChild->nextSibling->nextSibling;
        corps = param->nextSibling;
        if (init_table_symb(&table)) 
            return 3;
        switch (construit_tab(&table, param, 1, 1)) {
            case 2: return 2;
            case 3: return 3;
        }
        switch (construit_tab(&table, corps, 0, 1)) {
            case 2: return 2;
            case 3: return 3;
        }
        if (ajout_table(tables, table))
            return 3;
    }
    return 0;
}

/* Vérifie si une fonction d'entrée sortie est déjà ajoutée à la table */
static int deja_ajoute(TableSymb global, char *nom, Node *node) {
    for(int i = 0; i < global.nb_symb; i++) {
        if (!strcmp(nom, global.table[i].ident) && !global.table[i].info.fonct) {
            printf("semantic error : L:%d : %s déclaré en tant que variable\n", node->lineno, nom);
            return 2;
        } else if (!strcmp(nom, global.table[i].ident) && global.table[i].info.fonct)
            return 1;
    }
    return 0;
}

/* Ajoute un des fonctions d'entrée sortie à la table globale si elle est utilisée */
static int ajout_IO_fct(Tables *tables, char *nom, Node *node) {
    int type = 0;
    TableSymb table;
    switch (deja_ajoute(tables->tables[0], nom, node->firstChild)) {
        case 1: return 0;
        case 2: return 2;
    }
    if (init_table_symb(&table)) 
        return 3;
    if (!strcmp("getchar", nom))
        type = 1;
    else if (!strcmp("getint", nom)) {
        type = 2;
    } else if (!strcmp("putchar", nom)) {
        switch (ajout_symb(&table, 1, "c", 0, 0, 1, 1)) {
            case 2: return 2;
            case 3: return 3;
        }
    } else if (!strcmp("putint", nom)){
        switch (ajout_symb(&table, 2, "n", 0, 0, 1, 1)) {
            case 2: return 2;
            case 3: return 3;
        }
    }
    if (ajout_table(tables, table))
        return 3; 
    switch (ajout_symb(&(tables->tables[0]), type, nom, 0, 0, 1, 0)) {
        case 2: return 2;
        case 3: return 3;
    }
    return 0;
}

static int ajout_IO_bis(Tables *tables, Node *node) {
    if (!node) return 0;
    if (node->label == fonction && est_fct_IO(node->firstChild->ident)) {
        switch(ajout_IO_fct(tables, node->firstChild->ident, node)) {
            case 2: return 2;
            case 3: return 3; 
        };
    }
    // le noeud n'est pas une fonction
    switch(ajout_IO_bis(tables, node->firstChild)) {
        case 2: return 2;
        case 3: return 3;
    }
    switch(ajout_IO_bis(tables, node->nextSibling)) {
        case 2: return 2;
        case 3: return 3;
    }
    return 0;
}

/* Ajoute les fonctions d'entrée sortie à la table globale si elles sont utilisées */
static int ajout_IO(Tables *tables, Node *node) {
    Node *instr;
    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) {
        instr = child->firstChild->nextSibling->nextSibling->nextSibling->nextSibling;
        switch(ajout_IO_bis(tables, instr)) {
            case 2: return 2;
            case 3: return 3;
        }
    }
    return 0;
}

/* Construit les tables de symboles à partir de l'arbre abstrait */
int construit_tables_symb(Tables *tables, Node *arbre_abstrait) {
    init_tables(tables);
    switch (contruit_tab_var_globales(tables, arbre_abstrait->firstChild)) {
        case 2: return 2;
        case 3: return 3;
    } // variables globales
    switch(ajout_IO(tables, arbre_abstrait->firstChild->nextSibling)) {
        case 2: return 2;
        case 3: return 3;
    }
    switch (ajout_fonct(&(tables->tables[0]), arbre_abstrait->firstChild->nextSibling)) {
        case 2: return 2;
        case 3: return 3;
    }
    switch (construit_tab_fonct(tables, arbre_abstrait->firstChild->nextSibling)) {
        case 2: return 2;
        case 3: return 3;
    } // fonctions
    return 0;
}

/* Affiche les tables de symboles dans le terminal */
void aff_tables(Tables tables) {
    printf("\n");
    for(int i = 0; i < tables.nb_tab; i++) {
        printf("Table %d :\n", i);
        if (tables.tables[i].nb_symb == 0) printf("VIDE\n");
        for(int j = 0; j < tables.tables[i].nb_symb; j++) {
            printf("|");
            switch(tables.tables[i].table[j].type) {
                case 0: printf("Type : void, "); break;
                case 1: printf("Type : char, "); break;
                case 2: printf("Type : int, "); break;
                case 3: printf("Type : char array, "); break;
                case 4: printf("Type : int array, "); break;
            }
            printf("Ident : %s, ", tables.tables[i].table[j].ident);
            printf("Taille : %d, ", tables.tables[i].table[j].taille);
            if (i > 0)
                printf("Param : %s, ", tables.tables[i].table[j].info.param ? "oui" : "non");
            else
                printf("Fonct : %s, ", tables.tables[i].table[j].info.fonct ? "oui" : "non");
            printf("Adr : %ld\n", tables.tables[i].table[j].adr);
        }
        printf("\n");
    }
}