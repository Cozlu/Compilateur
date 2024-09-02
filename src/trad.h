#ifndef TRAD_H
#define TRAD_H

#include "tree.h"
#include "table.h"

/* Traduit un programme tpc en nasm */
int traduction(FILE * f, Tables *tables, Node *node);

#endif