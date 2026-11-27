/**
 * Project: IFJ26
 * @file bst.h
 * @brief rozhraní výškově vyváženého binárního vyhledávacího stromu (AVL)
 *
 * @author xsmekav00, Veronika Smékalová
 */

#ifndef BST_H
#define BST_H

#include <stdbool.h> // datový typ bool
#include <stdlib.h>	 // standardní typy a alokace

/**
 * @brief uzel AVL stromu
 */
typedef struct AVLNode
{
	char* key;
	void* data;
	int height;
	struct AVLNode* left_son;
	struct AVLNode* right_son;
} AVLNode;

/**
 * @brief inicializuje prázdný AVL strom
 *
 * @param node - ukazatel na kořen stromu
 */
void avl_inicialization(AVLNode** node);

/**
 * @brief vloží nebo přepíše záznam v AVL stromu a vyváží jej
 *
 * @param node - ukazatel na kořen stromu
 * @param key - klíč vkládaného uzlu
 * @param data - ukazatel na data uzlu
 */
void avl_insertion(AVLNode** node, const char* key, void* data);

/**
 * @brief uvolní uzly a klíče AVL stromu
 *
 * @param node - kořen stromu
 */
void avl_disposal(AVLNode* node);

/**
 * @brief vyhledá uzel podle klíče v AVL stromu
 *
 * @param node - kořen stromu
 * @param key - hledaný klíč
 * @return AVLNode* - nalezený uzel nebo NULL
 */
AVLNode* avl_search(AVLNode* node, const char* key);

#endif
