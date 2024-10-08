/* tree.h */

#ifndef TREE_H
#define TREE_H

typedef enum {
  program,
  declarations,
  type,
  ident,
  fonctions,
  fonction,
  parametres,
  instructions,
  ou,
  et,
  eq,
  ordre,
  addsub,
  divstar,
  non,
  num,
  charact,
  si,
  tantque,
  assign,
  retour,
  tableau
  /* list all other node labels, if any */
  /* The list must coincide with the string array in tree.c */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
} label_t;

typedef struct Node {
  label_t label;
  struct Node *firstChild, *nextSibling;
  int lineno;
  char byte;
  int num;
  char ident[64];
  int type;
  char comp[3];
} Node;

Node *makeNode(label_t label);
void addSibling(Node *node, Node *sibling);
void addChild(Node *parent, Node *child);
void deleteTree(Node*node);
void printTree(Node *node);

#define FIRSTCHILD(node) node->firstChild
#define SECONDCHILD(node) node->firstChild->nextSibling
#define THIRDCHILD(node) node->firstChild->nextSibling->nextSibling

#endif
