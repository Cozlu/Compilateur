#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "trad.h"
#include "sem.h"

#define reg_param "rdi", "rsi", "rdx", "rcx", "r8", "r9"

int nb_if = 0;
int nb_while = 0;
int nb_cond = 0;
int nb_return = 0;
int isMain = 0;
int isFunct = 0;
char * reg[6] = {reg_param};

static void rech_trad(FILE * f, TableSymb table, TableSymb global, Node *node);

/* Vérifie si la variable est locale ou globale */
static int est_locale(TableSymb table, TableSymb global, char *nom) {
    for (int i = 0; i < table.nb_symb; i++)
        if (!strcmp(nom, table.table[i].ident))
            return 1;
    for (int i = 0; i < global.nb_symb; i++)
        if (!strcmp(nom, global.table[i].ident))
            return 0;
    return 0;
}

/* Ecrit la traduction du déplacement de la valeur des variables dans rax */
static void ecrit_var(FILE * f, int adr, Node *node, int est_locale) {
    if(adr != -1) {
        if (node->label == tableau) {
            if (est_locale)
                fprintf(f, "mov rax, qword [rbp + %d + 8 * rax]\n", adr);
            else 
                fprintf(f, "mov rax, qword [global_var + %d + 8 * rax]\n", adr);
        } else {
            if (est_locale)
                fprintf(f, "mov rax, qword [rbp + %d]\n", adr);
            else
                fprintf(f, "mov rax, qword [global_var + %d]\n", adr);
        }
    } else if (node->label == num)
        fprintf(f, "mov rax, %d\n", node->num); // Ecriture de constante int
    else if (node->label == charact)
        fprintf(f, "mov rax, %d\n", node->byte); // Ecriture de constante char
    fprintf(f, "push rax\n");
}

/* Recherche à partir des tables de symboles l'adresse d'une variable */
static int rech_adr(TableSymb table, TableSymb global, char *ident) {
    int adr = -1;
    for (int i = 0; i < table.nb_symb; i++) // recherche table variables locales
        if (!strcmp(ident, table.table[i].ident)) {
            adr = table.table[i].adr;
            break;
        }
    if (adr != -1) // est var locale
        return adr;
    for (int i = 0; i < global.nb_symb; i++) // recherche table variables globales
        if (!strcmp(ident, global.table[i].ident))
            return global.table[i].adr; 
    return -1;
}

/* Ecrit en nasm la traduction d'une expression */
static void ecrit_expr(FILE * f, TableSymb table, TableSymb global, Node *node) {
    if (!node) return;
    if (node->label == tableau) {
        rech_trad(f, table, global, node->firstChild->nextSibling);
        fprintf(f, "pop rax\n");
        ecrit_var(f, rech_adr(table, global, node->firstChild->ident), node, est_locale(table, global, node->firstChild->ident));
    } else if(node->firstChild && node->firstChild->nextSibling) { // 2 opérandes
        rech_trad(f, table, global, node->firstChild);
        if (node->firstChild->label != fonction) // traduction du 2ème opérande déjà écrite par le case fonction dans rech_trad
            rech_trad(f, table, global, node->firstChild->nextSibling);
        switch(node->byte) {
            case '-':
                fprintf(f, "pop rcx\npop rax\nsub rax, rcx\npush rax\n");
                break;
            case '+':
                fprintf(f, "pop rcx\npop rax\nadd rax, rcx\npush rax\n"); 
                break;
            case '*':
                fprintf(f, "pop rcx\npop rax\nimul rax, rcx\npush rax\n"); 
                break;
            case '/':
                fprintf(f, "pop rcx\npop rax\nmov rdx, 0\nidiv rcx\npush rax\n");
                break;
            case '%':
                fprintf(f, "pop rcx\npop rax\nmov rdx, 0\nidiv rcx\npush rdx\n");
                break;
        }
    } else {
        if (strlen(node->ident)) // variable
            ecrit_var(f, rech_adr(table, global, node->ident), node, est_locale(table, global, node->ident));
        else // constante
            ecrit_var(f, -1, node, 0);
    }
}

/* Ecrit l'évaluation de la condition est met 1 sur la pile si elle est vraie, 0 sinon */
static void ecrit_cond(FILE * f, TableSymb table, TableSymb global, Node *node) {
    switch (node->label)
    {
    case 8: // ||
        ecrit_cond(f, table, global, node->firstChild);
        fprintf(f, "pop rax\ncmp rax, 0\n");
        fprintf(f, "jne label_%d\n", nb_cond);
        if (node->firstChild->label != fonction) // traduction du 2ème opérande déjà écrite par le case fonction dans rech_trad
            ecrit_cond(f, table, global, node->firstChild->nextSibling);
        fprintf(f, "jne label_%d\n", nb_cond);
        fprintf(f, "push 0\njmp label_%d\nlabel_%d:\npush 1\nlabel_%d:\n", nb_cond + 1, nb_cond, nb_cond + 1);
        nb_cond += 2;
        break;
    case 9: // &&
        ecrit_cond(f, table, global, node->firstChild);
        fprintf(f, "pop rax\ncmp rax, 0\n");
        fprintf(f, "je label_%d\n", nb_cond);
        if (node->firstChild->label != fonction)
            ecrit_cond(f, table, global, node->firstChild->nextSibling);
        fprintf(f, "je label_%d\n", nb_cond);
        fprintf(f, "push 1\njmp label_%d\nlabel_%d:\npush 0\nlabel_%d:\n", nb_cond + 1, nb_cond, nb_cond + 1);
        nb_cond += 2;
        break;
    case 10: // ==, !=
        ecrit_cond(f, table, global, node->firstChild);
        if (node->firstChild->label != fonction)
            ecrit_cond(f, table, global, node->firstChild->nextSibling);
        fprintf(f, "pop rcx\npop rax\ncmp rcx, rax\n");
        if(node->comp[0] == '=')
            fprintf(f, "je label_%d\n", nb_cond);
        else if(node->comp[0] == '!')
            fprintf(f, "jne label_%d\n", nb_cond);
        fprintf(f, "push 0\njmp label_%d\nlabel_%d:\npush 1\nlabel_%d:\n", nb_cond + 1, nb_cond, nb_cond + 1);
        nb_cond += 2;
        break;
    case 11: // <, <=, >, >=
        ecrit_cond(f, table, global, node->firstChild);
        if (node->firstChild->label != fonction)
            ecrit_cond(f, table, global, node->firstChild->nextSibling);
        fprintf(f, "pop rax\npop rcx\ncmp rcx, rax\n");
        if(node->comp[1]) { // <=, >=
            if(node->comp[0] == '>')
                fprintf(f, "jge label_%d\n", nb_cond);
            else
                fprintf(f, "jle label_%d\n", nb_cond);
        }
        else { // <, >
            if(node->comp[0] == '>')
                fprintf(f, "jg label_%d\n", nb_cond);
            else
                fprintf(f, "jl label_%d\n", nb_cond);
        }
        fprintf(f, "push 0\njmp label_%d\nlabel_%d:\npush 1\nlabel_%d:\n", nb_cond + 1, nb_cond, nb_cond + 1);
        nb_cond += 2;
        break;
    case 12: case 13: // +,-,*,/,% à évaluer
        ecrit_expr(f, table, global, node);
        break;
    case 14: // ! (not)
        ecrit_cond(f, table, global, node->firstChild);
        fprintf(f, "pop rax\ncmp rax, 0\n");
        fprintf(f, "je label_%d\n", nb_cond);
        fprintf(f, "push 0\njmp label_%d\nlabel_%d:\npush 1\nlabel_%d:\n", nb_cond + 1, nb_cond, nb_cond + 1);
        nb_cond += 2;
        break;
    default: // Constante ou Variable
        rech_trad(f, table, global, node);
        break;
    }
}

/* Ecrit la traduction d'un if simple ou if else en nasm */
static void ecrit_if(FILE * f, TableSymb table, TableSymb global, Node *node) {
    int cur_nb_if = nb_if; 
    nb_if += 1;
    if(!node) return;
    if(node->firstChild && node->firstChild->nextSibling && node->firstChild->nextSibling->nextSibling) {
        // If avec Else
        ecrit_cond(f, table, global, node->firstChild);
        fprintf(f, "pop rax\ncmp rax, 0\n");
        fprintf(f, "je else_%d\n", cur_nb_if);
        rech_trad(f, table, global, node->firstChild->nextSibling->firstChild);
        fprintf(f, "jmp after_if_%d\n", cur_nb_if);
        fprintf(f, "else_%d:\n", cur_nb_if);
        rech_trad(f, table, global, node->firstChild->nextSibling->nextSibling);
        fprintf(f, "after_if_%d:\n", cur_nb_if);
    } else {
        // If simple
        ecrit_cond(f, table, global, node->firstChild);
        fprintf(f, "pop rax\ncmp rax, 0\n");
        fprintf(f, "je after_if_%d\n", cur_nb_if);
        rech_trad(f, table, global, node->firstChild->nextSibling);
        fprintf(f, "after_if_%d:\n", cur_nb_if);
    }
}

/* Ecrit la traduction d'un while en nasm */
static void ecrit_while(FILE * f, TableSymb table, TableSymb global, Node *node) {
    int cur_nb_while = nb_while; 
    nb_while += 1;
    fprintf(f, "while_%d:\n", cur_nb_while);
    ecrit_cond(f, table, global, node->firstChild);
    fprintf(f, "pop rax\ncmp rax, 0\n");
    fprintf(f, "jne while_expr_%d\n", cur_nb_while);
    fprintf(f, "jmp after_while_%d\n", cur_nb_while);
    fprintf(f, "while_expr_%d:\n", cur_nb_while);
    rech_trad(f, table, global, node->firstChild->nextSibling);
    fprintf(f, "jmp while_%d\n", cur_nb_while);
    fprintf(f, "after_while_%d:\n", cur_nb_while);
}

/* Appelle la fonction adéquate selon le noeud rencontré dans l'arbre pour effectuer la traduction */
static void rech_trad(FILE * f, TableSymb table, TableSymb global, Node *node) {
    if (!node) return;
    switch(node->label) {
        case addsub:
        case divstar:
        case num:
        case charact:
        case ident:
        case tableau:
            ecrit_expr(f, table, global, node); 
            break;
        case ou:
        case et:
        case eq:
        case non:
        case ordre:
            ecrit_cond(f, table, global, node);
            break;
        case si:
            ecrit_if(f, table, global, node); 
            rech_trad(f, table, global, node->nextSibling);
            break;
        case tantque:
            ecrit_while(f, table, global, node); 
            rech_trad(f, table, global, node->nextSibling);
            break;
        case assign: {
            rech_trad(f, table, global, node->firstChild->nextSibling); // traduction de l'expression
            if (node->firstChild->label == tableau) { // traduction de la variable à qui on affecte l'expression
                int adr = rech_adr(table, global, node->firstChild->firstChild->ident);
                rech_trad(f, table, global, node->firstChild->firstChild->nextSibling);
                fprintf(f, "pop rax\n");
                if (est_locale(table, global, node->firstChild->firstChild->ident))
                    fprintf(f, "pop qword [rbp + %d + 8 * rax]\n", adr);
                else
                    fprintf(f, "pop qword [global_var + %d + 8 * rax]\n", adr);
            } else {
                int adr = rech_adr(table, global, node->firstChild->ident);
                if (est_locale(table, global, node->firstChild->ident))
                    fprintf(f, "pop qword [rbp + %d]\n", adr);
                else
                    fprintf(f, "pop qword [global_var + %d]\n", adr);
            }
            rech_trad(f, table, global, node->nextSibling);
            break;
        }
        case fonction:;
            char * name = node->firstChild->ident;
            int nb_params = 0, i = 0;
            for (Node *param = node->firstChild->nextSibling; param != NULL; param = param->nextSibling)
                nb_params++;
            Node **params = (Node **)malloc(sizeof(Node *)*nb_params);
            for (Node *param = node->firstChild->nextSibling; param != NULL; param = param->nextSibling, i++)
                params[i] = param; // stockage des noeuds de paramètres pour lecture inverse
            for (int j = nb_params - 1; j >= 0; j--) // lecture des paramètres dans l'ordre inverse (du dernier au 1er)
                rech_trad(f, table, global, params[j]); // empilement
            for (int j = 0; j < 6 && j < nb_params; j++) 
                fprintf(f, "pop %s\n", reg[j]); // placement des paramètres dans les registres
            if (!strcmp(name, "main"))
                fprintf(f, "mov r11, rsp\nsub rsp, 8\nand rsp, -16\nmov qword [rsp], r11\ncall _start\npop rsp\n");
            else
                fprintf(f, "mov r11, rsp\nsub rsp, 8\nand rsp, -16\nmov qword [rsp], r11\ncall %s\npop rsp\n", name);
            int ind = -1;
            for (int i = 0; i < global.nb_symb; i++)
                if (!strcmp(name, global.table[i].ident)) {
                    ind = i;
                    break;
                }
            if (ind != -1 && global.table[ind].type)
                fprintf(f, "push rax\n"); // stockage du résultat de la fonction
            rech_trad(f, table, global, node->nextSibling);
            break;
        case retour: 
            if (isMain) {
                rech_trad(f, table, global, node->firstChild);
                fprintf(f, "pop rdi\n");
                fprintf(f, "mov rax, 60\n");
                fprintf(f, "mov rsp, rbp\npop rbp\n");
                fprintf(f, "syscall\n");
                rech_trad(f, table, global, node->nextSibling);
            } else {
                rech_trad(f, table, global, node->firstChild);
                fprintf(f, "pop rax\n");
                rech_trad(f, table, global, node->nextSibling);
                fprintf(f, "jmp return_%d\n", nb_return);
            }
            break;
        default:
            rech_trad(f, table, global, node->firstChild);
            rech_trad(f, table, global, node->nextSibling);
    }
}

/* Ecrit l'allocation de mémoire statique pour les variables globales */
static void ecrit_static(FILE * f, TableSymb t) {
    // Allouer de la mémoire statique et initialiser
    fprintf(f, "section .bss\n");
    fprintf(f, "global_var: resb %ld\n", t.deplct);
}

/* Traduit la fonction putchar */
static void ecrit_putchar(FILE *f) {
    fprintf(f, "putchar :\npush rbp\nmov rbp, rsp\npush rdi\nmov rax, 1\n"
    "mov rdi, 1\nmov rsi, rsp\nmov rdx, 1\nsyscall\nmov rsp, rbp\npop rbp\nret\n");
}

/* Traduit la fonction putint */
static void ecrit_putint(FILE *f) {
    fprintf(f, "putint :\npush rbp\nmov rbp, rsp\npush r12\npush r13\ncmp rdi, 0\njge suite ; test négatif\nmov r12, rdi\nneg r12\n"
    "mov rdi, 45 ; afficher '-'\ncall putchar\nmov rdi, r12\nsuite :\nmov rdx, 0\nmov rax, 0\nmov r12, 10 ; init i à imax (10 chiffres dans un entier 4 octets)\n"
    "mov r13, 10 ; div par 10\nmov rax, rdi\nconv :\nmov rdx, 0\nidiv r13\nadd rdx, 48\nmov byte [digit_aff+r12], dl\ndec r12\ninc r14\ncmp rax, 0\njg conv\n"
    "ecrire : ; écrire les chiffres dans digit_aff\nmov dil, byte [digit_aff+r12+1]\nmov byte [digit_aff+r12+1], 0\ncall putchar\ninc r12\ncmp r12, 10\njl ecrire\n"
    "pop r13\npop r12\nmov rsp, rbp\npop rbp\nret\n");
}

/* Traduit la fonction getchar */
static void ecrit_getchar(FILE *f) {
    fprintf(f, "getchar :\npush rbp\nmov rbp, rsp\nsub rsp, 8\nmov rax, 0\nmov rdi, 0\n"
    "mov rsi, rsp\nmov rdx, 1\nsyscall\npop rax\nmov rsp, rbp\npop rbp\nret\n");
}

/* Traduit la fonction getint */
static void ecrit_getint(FILE *f) {
    fprintf(f, "getint : \npush rbp\nmov rbp, rsp\npush rbx\npush r12\npush r13\npush r14\nmov rax, 0\nmov rdi, 0\nmov rsi, digit_lect\nmov rdx, 10\nsyscall\n"
    "mov rbx, 0\nmov bl, byte [digit_lect] ; digit\nmov r12, 0 ; number\nmov r13, 0 ; adr\nmov r14, 0 ; pas négatif\ncmp bl, '-'\nje negatif\n"
    "cmp bl, '0' ; test si chiffre\njl err\ncmp bl, '9'\njg err\njmp est_nombre\nnegatif :\ninc r13\nmov bl, byte [digit_lect+r13] ;digit\nmov byte [digit_lect+r13], 0\n"
    "mov r14, 1 ;est négatif\nest_nombre :\nsub rbx, 48\nimul r12, 10\nadd r12, rbx\ninc r13\nmov bl, byte [digit_lect+r13] ;digit\nmov byte [digit_lect+r13], 0\ncmp bl, '0'\njl negatif_getint_vrai\n"
    "cmp bl, '9'\njg negatif_getint_vrai\njmp est_nombre\nnegatif_getint_vrai :\ncmp r14, 1 ; si nb négatif\njne fin\nneg r12\nfin:\nmov rax, r12\njmp apres\nerr:\n"
    "mov rax, 5\napres:\npop r14\npop r13\npop r12\npop rbx\nmov rsp, rbp\npop rbp\nret\n");
}

/* Ecrit les fonctions d'entrée sortie si elles sont utilisées */
void ecrit_IO(FILE * f, TableSymb global) {
    int putchar_ecrit = 0;
    fprintf(f, "section .data\ndigit_aff: db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\ndigit_lect: db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n");
    fprintf(f, "section .text\n");
    for(int j = 0; j < global.nb_symb; j++) {
        if (global.table[j].info.fonct) {
            if (!strcmp(global.table[j].ident, "putchar") && !putchar_ecrit) {
                ecrit_putchar(f);
                putchar_ecrit = 1;
            } else if (!strcmp(global.table[j].ident, "putint")) {
                if (!putchar_ecrit) {
                    ecrit_putchar(f);
                    putchar_ecrit = 1;
                }
                ecrit_putint(f);
            } else if (!strcmp(global.table[j].ident, "getint"))
                ecrit_getint(f);
            else if (!strcmp(global.table[j].ident, "getchar"))
                ecrit_getchar(f);
        }
    }
}

/* Vérifie s'il faut écrire les fonctions d'entrée sortie */
static int ecrire_IO(TableSymb global) {
    for(int j = 0; j < global.nb_symb; j++) {
        if (global.table[j].info.fonct)
            if (!strcmp(global.table[j].ident, "putchar") || !strcmp(global.table[j].ident, "putint") ||
                !strcmp(global.table[j].ident, "getint") || !strcmp(global.table[j].ident, "getchar")) {
                return 1;
        }
    }
    return 0;
}

/* Place les paramètres contenus dans les registres au même endroit que les variables locales */
static void ecrire_params(FILE *f, TableSymb table, TableSymb global, Node *node) {
    int i = 0;
    for (Node *param = node->firstChild; param != NULL; param = param->nextSibling, i++) {
        if (param->label == type && param->type == 0) break;
        if (i < 6) { // paramètres contenus dans les registres
            if (param->label == tableau)
                fprintf(f, "mov qword [rbp + %d], %s\n", rech_adr(table, global, param->firstChild->nextSibling->ident), reg[i]);
            else
                fprintf(f, "mov qword [rbp + %d], %s\n", rech_adr(table, global, param->firstChild->ident), reg[i]);
        }
    }
}

/* Effectue la traduction d'un programme TPC en nasm tout en signalant les erreurs sémantiques */
int traduction(FILE * f, Tables *tables, Node *node) {
    char *nom_fonc;
    Node *instr, *params;
    ecrit_static(f, tables->tables[0]);
    fprintf(f, "global _start\n");
    if (ecrire_IO(tables->tables[0]))
        ecrit_IO(f, tables->tables[0]);
    else
        fprintf(f, "section .text\n");
    if (verif_main(tables->tables[0])) return 1;
    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) { // parcours des fonctions du noeud fonctions
        nom_fonc = child->firstChild->nextSibling->ident;
        instr = child->firstChild->nextSibling->nextSibling->nextSibling->nextSibling;
        params = child->firstChild->nextSibling->nextSibling;
        if (verif_decl(i_fonct(nom_fonc, tables), tables, instr)) return 1;
        if (verif_type(i_fonct(nom_fonc, tables), tables, instr)) return 1;
        int i_table = i_fonct(nom_fonc, tables);
        if (!strcmp(nom_fonc, "main")) {
            isMain = 1;
            isFunct = 0;
            fprintf(f, "_start:\n");
            fprintf(f, "push rbp\nmov rbp, rsp\n");
            fprintf(f, "sub rsp, %ld\n", tables->tables[i_table].deplct);
            ecrire_params(f, tables->tables[i_table], tables->tables[0], params);
            rech_trad(f, tables->tables[i_table], tables->tables[0], instr);
        } else {
            isMain = 0;
            isFunct = 1;
            fprintf(f, "%s:\n", nom_fonc);
            fprintf(f, "push rbp\nmov rbp, rsp\n");
            fprintf(f, "sub rsp, %ld\n", tables->tables[i_table].deplct);
            ecrire_params(f, tables->tables[i_table], tables->tables[0], params);
            rech_trad(f, tables->tables[i_table], tables->tables[0], instr); 
            fprintf(f, "return_%d:\nmov rsp, rbp\npop rbp\nret\n", nb_return);
        }
        nb_return++;
    }
    return 0;
}
