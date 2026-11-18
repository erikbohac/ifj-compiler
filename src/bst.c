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

static int height(AVLNode *node){
    return node ? node->height : -1;
}

static int balance(AVLNode *node){
    return node ? height(node->left_son) - height(node->right_son) : 0;
}

static int comprare_keys(const char *key1, const char *key2){
    return strcmp(key1, key2);
}

static void update_height(AVLNode *node){
    if (node != NULL)
        node->height = MAX(height(node->left_son), height(node->right_son)) + 1;
}

void avl_inicialization (AVLNode **node){
    *node = NULL;
}

static AVLNode *rotation_right(AVLNode *y){
    AVLNode *x = y->left_son;

    y->left_son = x->right_son;
    x->right_son = y;

    update_height(y);
    update_height(x);

    return x;
}

static AVLNode *rotation_left(AVLNode *x){
    AVLNode *y = x->right_son;

    x->right_son = y->left_son;
    y->left_son = x;

    update_height(x);
    update_height(y);

    return y;
}

static AVLNode *avl_insertion_recrusive(AVLNode *node, const char *key, void *data){
    if (node == NULL){
        AVLNode *new_node = (AVLNode *)malloc(sizeof(AVLNode));

        if (new_node == NULL)
            return NULL;

        new_node->key = (char *)malloc(strlen(key) + 1);

        if (new_node->key != NULL)
            strcpy(new_node->key, key);

        new_node->data = data;
        new_node->height = 0;
        new_node->left_son = NULL;
        new_node->right_son = NULL;
        return new_node;
    }

    if(comprare_keys(key, node->key) < 0)
        node->left_son = avl_insertion_recrusive(node->left_son, key, data);

    else if(comprare_keys(key, node->key) > 0)
        node->right_son = avl_insertion_recrusive(node->right_son, key, data);

    else{
        node->data = data;
        return node;
    }

    update_height(node);

    int balance_factor = balance(node);

    if((balance_factor > 1) && (comprare_keys(key, node->left_son->key) < 0))
        return rotation_right(node);

    else if((balance_factor > 1) && (comprare_keys(key, node->left_son->key) > 0)){
        node->left_son = rotation_left(node->left_son);
        return rotation_right(node);
    }

    else if((balance_factor < -1) && (comprare_keys(key, node->right_son->key) > 0))
        return rotation_left(node);

    else if((balance_factor < -1) && (comprare_keys(key, node->right_son->key) < 0)){
        node->right_son = rotation_right(node->right_son);
        return rotation_left(node);
    }

    return node;
}

void avl_insertion(AVLNode **node, const char *key, void *data){
    *node = avl_insertion_recrusive(*node, key, data);
}

void avl_disposal(AVLNode **node){
    
}

AVLNode *avl_search(AVLNode *node, const char *key){
    if (node == NULL)
        return NULL;

    if (ccomprare_keys(key, node->key) == 0)
        return node;

    else if (comprare_keys(key, node->key) > 0)
        return avl_search(node->right_son, key);

    else
        return avl_search(node->left_son, key);
}
