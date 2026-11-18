/**
 * Project: IFJ26
 * @file bst.h 
 * 
 * @author xsmekav00, Veronika Smékalová
*/

#ifndef BST_H
#define BST_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct AVLNode {
    char *key;
    void *data;
    int height;
    struct AVLNode *left_son;
    struct AVLNode *right_son;
} AVLNode;

void avl_inicialization(AVLNode **node);
void avl_insertion(AVLNode **node, const char *key, void *data);
AVLNode *avl_search(AVLNode *node, const char *key);
void avl_disposal(AVLNode **node);

#endif
