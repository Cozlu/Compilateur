#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "../src/table.h"
#include "../src/trad.h"
#include "../src/tree.h"
#include "../obj/parser.tab.h"

int usage(char * prog) {
    fprintf(stderr, "Mauvais usage : %s [OPTIONS]\n", prog);
    fprintf(stderr, "-t, --tree : Affiche l'arbre abstrait\n");
    fprintf(stdout, "-s, --symtab : Affiche les tables de symbole\n");
    fprintf(stderr, "-h, --help : Affiche une aide pour utiliser le programme\n");
    return 3;
}

int help(char * prog) {
	fprintf(stdout,"--------------------------------------------------------------------------------------\n");
    fprintf(stdout, "Réalise l'analyse syntaxique et la compilation du fichier passé dans l'entrée standard\n");
   	fprintf(stdout,"--------------------------------------------------------------------------------------\n");

	fprintf(stdout, "usage : %s [OPTIONS]\n", prog);
    fprintf(stdout, "-t, --tree : Affiche l'arbre abstrait du fichier à compile\n");
    fprintf(stdout, "-s, --symtab : Affiche les tables de symbole du fichier à compiler\n");
    fprintf(stdout, "-h, --help : Affiche une aide pour utiliser le programme\n\n");
    fprintf(stdout, "\
   exemple : \n\
   %s -t < fichier.tpc\n\
   %s --help\n", prog, prog);
   fprintf(stdout,"--------------------------------------------------------------------------------------\n");
    return 3;
}

int options(int argc, char* argv[], int *aff_arbre, int *aff_tab) {
    int opt;
    static struct option long_opt[] = {
        {"tree", no_argument, 0, 't'},
        {"symtabs", no_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };
    while((opt = getopt_long(argc, argv, "ths", long_opt, NULL)) != -1) {
        switch(opt) {
            case 't' : *aff_arbre = 1; break;
            case 's' : *aff_tab = 1; break;
            case 'h' : return help(argv[0]);
            case '?' : return 2;
            default : return 2;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int opt;
    if (argc > 2)
        return usage(argv[0]);
    int aff_arbre = 0;
    int aff_tab = 0;
    opt = options(argc, argv, &aff_arbre, &aff_tab);
    if (opt) return 3;

    Node * arbre_abstrait = NULL;
    if (yyparse(&arbre_abstrait)) return 1;
    if (aff_arbre) printTree(arbre_abstrait); 

    Tables tables;
    switch (construit_tables_symb(&tables, arbre_abstrait)) {
        case 2 : return 2;
        case 3 :
            printf("erreur : Mémoire insufisante\n");
            free(tables.tables);
            deleteTree(arbre_abstrait);
            return 3;
    }
    if (aff_tab) aff_tables(tables);

    FILE *asmf;
    asmf = fopen("_anonymous.asm", "w");
    if (!asmf) return 3;
    /* Traduction du programme à partir du noeud fonctions */
    if (traduction(asmf, &tables, arbre_abstrait->firstChild->nextSibling)) {
        fclose(asmf);
        free(tables.tables);
        deleteTree(arbre_abstrait);
        return 2;
    }
    fclose(asmf);
    free(tables.tables);
    deleteTree(arbre_abstrait);
    return 0;
}
