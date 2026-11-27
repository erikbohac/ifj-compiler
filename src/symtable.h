/**
 * Project: IFJ26
 * @file symtable.h
 * @brief rozhraní tabulky symbolů nad AVL stromem
 *
 * @author xsmekav00, Veronika Smékalová
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h" // datové typy a uzly AST
#include "bst.h" // podkladový AVL strom

#include <stdbool.h> // datový typ bool
#include <stdlib.h>	 // standardní typy a alokace

/**
 * @brief druh symbolu v tabulce symbolů
 */
typedef enum
{
	TAG_VAR,
	TAG_VAL,
	TAG_FUNCTION
} SymbolTag;

/**
 * @brief metadata uložená pro každý symbol v tabulce
 */
typedef struct
{
	DataType type;
	SymbolTag tag;
	bool is_inicialized;
	int line_number;

	int parametrs_count;
	DataType* parametrs_types;
} SymbolData;

/**
 * @brief inicializuje prázdnou tabulku symbolů
 *
 * @param node - ukazatel na kořen tabulky symbolů
 */
void table_inicialization(AVLNode** node);

/**
 * @brief vloží nový symbol nebo přepíše existující záznam v tabulce symbolů
 *
 * @param node - ukazatel na kořen tabulky symbolů
 * @param key - název symbolu
 * @param type - datový typ symbolu
 * @param tag - druh symbolu
 * @param line - řádek v kódu
 * @return bool - true při úspěchu, false při chybě alokace
 */
bool table_insertion(AVLNode** node, const char* key, DataType type, SymbolTag tag, int line);

/**
 * @brief vyhledá symbol v tabulce podle klíče
 *
 * @param node - kořen tabulky symbolů
 * @param key - hledaný název symbolu
 * @return AVLNode* - nalezený uzel nebo NULL
 */
AVLNode* table_search(AVLNode* node, const char* key);

/**
 * @brief uvolní celou tabulku symbolů včetně dat a vynuluje kořen
 *
 * @param node - ukazatel na kořen tabulky symbolů
 */
void table_free(AVLNode** node);

#endif
