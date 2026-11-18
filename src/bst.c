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

static int height(AVLNode *node){
    return node ? node->height : -1;
}

static int balance_factor(AVLNode *node){
    return node ? height(node->left_son) - height(node->right_son) : 0;
}

static void rotation_right(AVLNode *y){
    AVLNode *x = y->left_son;

    y->left_son = x->right_son;
    x->right_son = y;

    y->height = MAX(height(y->left_son), height(y->right_son)) + 1;
    x->height = MAX(height(x->left_son), height(x->right_son)) + 1;

    return x;
}

static void rotation_left(AVLNode *x){
    AVLNode *y = x->right_son;

    x->right_son = y->left_son;
    y->left_son = x;

    x->height = MAX(height(x->left_son), height(x->right_son)) + 1;
    y->height = MAX(height(y->left_son), height(y->right_son)) + 1;

    return y;
}

void avl_insertion(AVLNode **node, const char *key, void *data){

}

AVLNode *avl_search(AVLNode *node, const char *key){
    if (node == NULL)
        return NULL;

    int compare_keys = strcmp (key, node->key);

    if(compare_keys == 0)
        return node;
    
    else if (compare_keys > 0)
        return avl_search(node->right_son, key);
    
    else
        return avl_search(node->left_son, key);
}

void avl_disposal(AVLNode **node){

}
