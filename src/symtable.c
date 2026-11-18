/**
 * Project: IFJ26
 * @file symtable.c 
 * 
 * @author xsmekav00, Veronika Smékalová
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

void table_inicialization(AVLNode **node){
    avl_inicialization(node);
}

void table_insertion(AVLNode **node, const char *key, DataType type, SymbolTag tag, int line){
    SymbolData *data = malloc(sizeof(SymbolData));

    if(data == NULL)
        return false;

    data->type = type;
    data->tag = tag;
    data->parametrs_types = NULL;
    data->parametrs_count = 0;
    data->line_number = line;
    data->is_inicialized = false;

    AVLNode *already_exists = table_search(*node, key);
    if (already_exists != NULL){
        SymbolData *vintage_data = (SymbolData*)already_exists->data;

        if (vintage_data != NULL){
            free(vintage_data->parametrs_types);
            free(vintage_data);
        }

        already_exists->data = data;
    }

    else
        avl_insertion(node, key, data);
}

void table_free_recrusive(AVLNode *node){
    if (node != NULL){
        table_free_recrusive(node->left_son);
        table_free_recrusive(node->right_son);

        if (node->data != NULL){
            SymbolData * data = (SymbolData*)node->data;
            free(data->parametrs_types);
            free(data);
        }

        free(node->key);
        free(node);
    }
}

void table_free(AVLNode **node){
    if (node != NULL && *node != NULL){
        table_free_recrusive(*node);
        *node = NULL;
    }
}

AVLNode *table_search(AVLNode *node, const char *key){
    return avl_search(node, key);
}
