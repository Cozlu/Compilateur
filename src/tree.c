/* tree.c */
#include "tree.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
extern int num_ligne;       /* from lexer */

static const char *StringFromLabel[] = {
  "program",
  "declarations",
  "type",
  "ident",
  "functions",
  "function",
  "parameters",
  "instructions",
  "or",
  "and",
  "eq",
  "order",
  "addsub",
  "divstar",
  "not",
  "num",
  "charact",
  "if",
  "while",
  "assign",
  "return",
  "array"
  /* list all other node labels, if any */
  /* The list must coincide with the label_t enum in tree.h */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
};

Node *makeNode(label_t label) {
  Node *node = malloc(sizeof(Node));
  if (!node) {
    printf("Run out of memory\n");
    exit(1);
  }
  node->label = label;
  node-> firstChild = node->nextSibling = NULL;
  node->lineno=num_ligne;
  return node;
}

void addSibling(Node *node, Node *sibling) {
  Node *curr = node;
  while (curr->nextSibling != NULL) {
    curr = curr->nextSibling;
  }
  curr->nextSibling = sibling;
}

void addChild(Node *parent, Node *child) {
  if (parent->firstChild == NULL) {
    parent->firstChild = child;
  }
  else {
    addSibling(parent->firstChild, child);
  }
}

void deleteTree(Node *node) {
  if (node->firstChild) {
    deleteTree(node->firstChild);
  }
  if (node->nextSibling) {
    deleteTree(node->nextSibling);
  }
  free(node);
}

void printTree(Node *node) {
  static bool rightmost[128]; // tells if node is rightmost sibling
  static int depth = 0;       // depth of current node
  for (int i = 1; i < depth; i++) { // 2502 = vertical line
    printf(rightmost[i] ? "    " : "\u2502   ");
  }
  if (depth > 0) { // 2514 = L form; 2500 = horizontal line; 251c = vertical line and right horiz 
    printf(rightmost[depth] ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ");
  }
  switch(node->label) {
    case type: 
      switch(node->type) {
        case 0: printf("void"); break;
        case 1: printf("char"); break;
        case 2: printf("int"); break;
      }
    case ident: printf("%s", node->ident); break;
    case num: printf("%d", node->num); break;
    case charact: switch (node->byte) {
      case '\n': printf("\\n"); break;  
      case '\t': printf("\\t"); break;  
      case '\'': printf("\\'"); break;  
      default:
        printf("%c", node->byte); break;
      } break;
    case addsub: case divstar: printf("%c", node->byte); break;
    case eq: case ordre: printf("%s", node->comp); break;
    default: printf("%s", StringFromLabel[node->label]);
  }
  printf("\n");
  depth++;
  for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) {
    rightmost[depth] = (child->nextSibling) ? false : true;
    printTree(child);
  }
  depth--;
}
