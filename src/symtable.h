/**
 * Project: IFJ26
 * @file symtable.h 
 * 
 * @author xsmekav00, Veronika Smékalová
*/

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stdlib.h>
#include "bst.h"

typedef enum{
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOLEAN
} DataType;

typedef enum{
    TAG_VAR,
    TAG_VAL,
    TAG_FUNCTION
} SymbolTag;

typedef struct{
    DataType type;
    SymbolTag tag;
    bool is_inicialized;
    int line_number;

    int parametrs_count;
    DataType *parametrs_types;
} SymbolData;

void table_inicialization(AVLNode **node);
void table_insertion(AVLNode **node, const char *key, DataType type, SymbolTag tag, int line);
AVLNode *table_search(AVLNode *node, const char *key);
void table_free(AVLNode **node);

#endif
