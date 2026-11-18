/**
 * Project: IFJ26
 * @file bst.c 
 * 
 * @author xsmekav00, Veronika Smékalová
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bst.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))

void avl_inicialization (AVLNode **node){
    *node = NULL;
}

//void avl_insertion(AVLNode **node, const char *key, void *data)

AVLNode *avl_search(AVLNode *node, const char *key){
    if (node == NULL){
        return NULL;
    }

    int compare_keys = strcmp (key, node->key);

    if(compare_keys == 0){
        return node;
    }
    
    else if (compare_keys > 0){
        return avl_search(node->right_son, key);
    } 
    
    else{
        return avl_search(node->left_son, key);
    }

}

//void avl_disposal(AVLNode **node)
