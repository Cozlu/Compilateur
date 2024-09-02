#include <stdio.h>
#include <string.h>
#include "sem.h"

/* Vérifie si la variable ou la fonction est déclarée dans la table */
static int est_declare(int i_table, Tables *tables, char *nom_var) {
    TableSymb table = tables->tables[i_table];
    for (int i = 0; i < table.nb_symb; i++) {
        if (!strcmp(nom_var, table.table[i].ident))
            return 1;
    }
    return 0;
}

/* Vérifie les déclarations des variables et fonctions utilisées */
int verif_decl(int i_table, Tables *tables, Node *node) {
    if (!node) return 0;
    if (node->label == ident && !est_declare(i_table, tables, node->ident) && !est_declare(0, tables, node->ident)) { // vérif dans table fct et table var globales
        printf("semantic error : L:%d : %s n'est pas déclaré\n", node->lineno, node->ident);
        return 1;
    }
    return verif_decl(i_table, tables, node->firstChild) || verif_decl(i_table, tables, node->nextSibling);
}

/* Cherche et renvoie le type de la variable nom_var à partir d'une table */
static int cherche_type_var_bis(TableSymb table, char *nom_var, int est_case, int est_param) {
    for (int i = 0; i < table.nb_symb; i++)
        if (!strcmp(nom_var, table.table[i].ident)) {
            if (table.table[i].type > 2) {
                if (!est_case && est_param)
                    return table.table[i].type;
                if (!est_case && !est_param)
                    return -1; // ident de tableau utilisé sans []
                if (est_case) // case tableau
                    return table.table[i].type - 2;
            }
            return table.table[i].type;
        }
    return 0;
}

/* Cherche et renvoie le type de la variable nom_var à partir des tables de symboles */
static int cherche_type_var(int i_table, Tables *tables, char *nom_var, int est_case, int est_param) {
    TableSymb var_globales = tables->tables[0];
    TableSymb table = tables->tables[i_table];
    int type_local = cherche_type_var_bis(table, nom_var, est_case, est_param);
    if (!type_local)
        return cherche_type_var_bis(var_globales, nom_var, est_case, est_param);
    return type_local;
}
 
/* Evalue le type d'une expression à partir des tables et de l'arbre abstarit */
static int eval_expr(int i_table, Tables *tables, Node *node, int est_case, int est_param) {
    int type = 0, se1, se2, ret;
    if (!node) return 0;
    if (node->label == fonction)
        return cherche_type_var_bis(tables->tables[0], node->firstChild->ident, 0, 0);
    if (!node->firstChild) { // variable, paramètre ou constante
        switch (node->label) {
            case ident:
                type = cherche_type_var(i_table, tables, node->ident, est_case, est_param);
                if (est_case) {
                    ret = eval_expr(i_table, tables, node->nextSibling, 0, est_param);
                    if (ret == -1) // remonter erreur sémantique
                        return -1;
                }
                if (type == -1) // ident de tableau utilisé sans []
                    printf("semantic error : L:%d : %s est utilisé sans opérateur []\n", node->lineno, node->ident);
                return type;
            case num: return 2;
            case charact: return 1;
            default: break;
        }
    } 
    if (node->label == tableau)
        return eval_expr(i_table, tables, node->firstChild, 1, 0);
    se1 = eval_expr(i_table, tables, node->firstChild, est_case, 0);
    se2 = eval_expr(i_table, tables, node->firstChild->nextSibling, est_case, 0);
    if (se1 == -1 || se2 == -1) // remonter erreur sémantique
        return -1;
    return 2;
}

int i_table_vers_i_symb(int i_table, Tables *tables) {
    TableSymb global = tables->tables[0];
    return global.nb_symb - 1 - (tables->nb_tab - 1 - i_table);
}

int i_fonct(char *nom, Tables *tables) {
    int i_fonct_tab = -1;
    TableSymb global = tables->tables[0];
    for (int i = 0; i < global.nb_symb; i++)
        if (!strcmp(nom, global.table[i].ident)) {
            i_fonct_tab = i;
            break;
        }
    return i_fonct_tab + tables->nb_tab - global.nb_symb;
}

/* Compte le nombre de paramètres qu'une fonction accepte */
static int compte_params(TableSymb table) {
    int nb_params = 0;
    for (int i = 0; i < table.nb_symb; i++)
        if (table.table[i].info.param)
            nb_params++;
    return nb_params;
}

/* Vérifie le type et le nombre de paramètres utilisés lors de l'appel à une fonction */
static int verif_params(int i_table, Tables *tables, Node *node) {
    int i = 0, type_param, type_expr, cnt_params = 0;
    TableSymb table = tables->tables[i_fonct(node->ident, tables)];
    int nb_params = compte_params(table);
    for (Node *param = node->nextSibling; param != NULL; param = param->nextSibling)
        cnt_params++;
    if (cnt_params != nb_params) {
        printf("semantic error : L:%d : nombre de paramètres incorrect, attendu : %d, donné : %d\n", node->lineno, nb_params, cnt_params);
        return 1;
    }
    for (Node *param = node->nextSibling; param != NULL; param = param->nextSibling, i++) { // vérif du type des params
        type_param = table.table[i].type;
        if (param->label == tableau) 
            type_expr = eval_expr(i_table, tables, param->firstChild, 1, 1);
        else 
            type_expr = eval_expr(i_table, tables, param, 0, 1);
        if (type_expr == -1) // remonter erreur sémantique
            return 1;
        if (!((type_param == 2 && type_expr == 1) || (type_param == 1 && type_expr == 2)) && type_param != type_expr) {
            switch(type_param) {
                case 1: printf("semantic error : L:%d : type du paramètre différent de celui attendu : char\n", node->lineno); break;
                case 2: printf("semantic error : L:%d : type du paramètre différent de celui attendu : int\n", node->lineno); break;
                case 3: printf("semantic error : L:%d : type du paramètre différent de celui attendu : char array\n", node->lineno); break;
                case 4: printf("semantic error : L:%d : type du paramètre différent de celui attendu : int array\n", node->lineno); break;
            }
            return 1;
        }
    }
    return 0;
}

int verif_type(int i_table, Tables *tables, Node *node) {
    int type_LValue = 0;
    int type_expr = 0;
    int type_fct;
    if (!node) return 0;
    if (node->label == fonction)
        return verif_params(i_table, tables, node->firstChild);
    if (node->label == retour) {
        TableSymb globale = tables->tables[0];
        type_fct = globale.table[i_table_vers_i_symb(i_table, tables)].type;
        if (!node->firstChild) // return vide (return;)
            type_expr = 0;
        else if (node->firstChild->label == tableau) 
            type_expr = eval_expr(i_table, tables, node->firstChild->firstChild, 1, 0);
        else
            type_expr = eval_expr(i_table, tables, node->firstChild, 0, 0);
        if (type_fct == 1 && type_expr == 2)
            printf("warning : L:%d : type int renvoyé par une fonction de type char\n", node->lineno);
        else if (!(type_fct == 2 && type_expr == 1) && type_fct != type_expr) {
            printf("semantic error : L:%d : type de retour différent du type de l'expression retournée\n", node->lineno);
            return 1;
        }
    }
    if (node->label == si || node->label == tantque) {
        if (node->firstChild->label == tableau) 
            type_expr = eval_expr(i_table, tables, node->firstChild->firstChild, 1, 0);
        else
            type_expr = eval_expr(i_table, tables, node->firstChild, 0, 0);
    }
    if (node->label == assign) {
        if (node->firstChild->label == tableau)
            type_LValue = cherche_type_var(i_table, tables, node->firstChild->firstChild->ident, 1, 0);
        else
            type_LValue = cherche_type_var(i_table, tables, node->firstChild->ident, 0, 0);
        if (node->firstChild->nextSibling->label == tableau) 
            type_expr = eval_expr(i_table, tables, node->firstChild->nextSibling->firstChild, 1, 0);
        else
            type_expr = eval_expr(i_table, tables, node->firstChild->nextSibling, 0, 0);
        if (type_expr == 0) {
           printf("semantic error : L:%d : affectation d'un type void à une variable\n", node->lineno);
           return 1;
        }
    }
    if (type_expr == -1 || type_LValue == -1) // remonter erreur sémantique
        return 1;
    if (type_LValue != type_expr && type_LValue == 1)
        printf("warning : L:%d : type int assigé à variable de type char\n", node->lineno);
    return verif_type(i_table, tables, node->firstChild) || verif_type(i_table, tables, node->nextSibling);
}

int verif_main(TableSymb global) {
    for (int i = 0; i < global.nb_symb; i++) {
        if (global.table[i].info.fonct && !strcmp("main", global.table[i].ident)) {
            switch(global.table[i].type) {
                case 0:
                    printf("semantic error : la fonction main n'est pas correctement typée, attendu : int, donné : void\n");
                    return 1;
                case 1: 
                    printf("semantic error : la fonction main n'est pas correctement typée, attendu : int, donné : char\n");
                    return 1;
                case 2: return 0;
                default:
                    printf("semantic error : la fonction main n'est pas correctement typée, attendu : int\n");
                    return 1;
            }
        }
    }
    printf("semantic error : la fonction main n'est pas déclarée\n");
    return 1;
}