/**
 * Project: IFJ26
 * @file bst.c
 * @brief implementace AVL stromu pro vyhledávání symbolů
 *
 * @author xsmekav00, Veronika Smékalová
 */

#include "bst.h" // rozhraní AVL stromu

#include <stdbool.h> // datový typ bool
#include <stdio.h>	 // standardní vstup/výstup
#include <stdlib.h>	 // dynamická alokace paměti (malloc, free)
#include <string.h>	 // práce s řetězci (strcmp, strcpy, strlen)

#define MAX(x, y) ((x) > (y) ? (x) : (y)) // maximum ze dvou hodnot

/**
 * @brief vrátí výšku uzlu stromu
 *
 * @param node - uzel stromu
 * @return int - výška uzlu nebo -1 pro NULL
 */
static int height(AVLNode* node)
{
	return node ? node->height : -1;
}

/**
 * @brief spočítá faktor vyvážení uzlu (levý minus pravý podstrom)
 *
 * @param node - uzel stromu
 * @return int - faktor vyvážení uzlu
 */
static int balance(AVLNode* node)
{
	return node ? height(node->left_son) - height(node->right_son) : 0;
}

/**
 * @brief porovná dva klíče lexikograficky
 *
 * @param key1 - první klíč
 * @param key2 - druhý klíč
 * @return int - záporné číslo, 0 nebo kladné číslo dle strcmp
 */
static int comprare_keys(const char* key1, const char* key2)
{
	return strcmp(key1, key2);
}

/**
 * @brief přepočítá výšku uzlu podle výšek jeho potomků
 *
 * @param node - aktualizovaný uzel
 */
static void update_height(AVLNode* node)
{
	if (node != NULL)
		node->height = MAX(height(node->left_son), height(node->right_son)) + 1;
}

/**
 * @brief nastaví ukazatel na kořen stromu na NULL
 *
 * @param node - ukazatel na kořen stromu
 */
void avl_inicialization(AVLNode** node)
{
	*node = NULL;
}

/**
 * @brief provede pravou rotaci podstromu kolem uzlu y
 *
 * @param y - kořen rotovaného podstromu
 * @return AVLNode* - nový kořen podstromu po rotaci
 */
static AVLNode* rotation_right(AVLNode* y)
{
	AVLNode* x = y->left_son;

	y->left_son = x->right_son;
	x->right_son = y;

	update_height(y);
	update_height(x);

	return x;
}

/**
 * @brief provede levou rotaci podstromu kolem uzlu x
 *
 * @param x - kořen rotovaného podstromu
 * @return AVLNode* - nový kořen podstromu po rotaci
 */
static AVLNode* rotation_left(AVLNode* x)
{
	AVLNode* y = x->right_son;

	x->right_son = y->left_son;
	y->left_son = x;

	update_height(x);
	update_height(y);

	return y;
}

/**
 * @brief rekurzivně vloží prvek do AVL stromu a provede případné vyvážení
 *
 * @param node - aktuální uzel podstromu
 * @param key - vkládaný klíč
 * @param data - data přiřazená ke klíči
 * @return AVLNode* - nový kořen vyváženého podstromu
 */
static AVLNode* avl_insertion_recrusive(AVLNode* node, const char* key, void* data)
{
	if (node == NULL)
	{
		AVLNode* new_node = (AVLNode*)malloc(sizeof(AVLNode));

		if (new_node == NULL)
			return NULL;

		new_node->key = (char*)malloc(strlen(key) + 1);

		if (new_node->key != NULL)
			strcpy(new_node->key, key);

		new_node->data = data;
		new_node->height = 0;
		new_node->left_son = NULL;
		new_node->right_son = NULL;
		return new_node;
	}

	if (comprare_keys(key, node->key) < 0)
		node->left_son = avl_insertion_recrusive(node->left_son, key, data);

	else if (comprare_keys(key, node->key) > 0)
		node->right_son = avl_insertion_recrusive(node->right_son, key, data);

	else
	{
		node->data = data;
		return node;
	}

	update_height(node);

	int balance_factor = balance(node);

	if ((balance_factor > 1) && (comprare_keys(key, node->left_son->key) < 0))
		return rotation_right(node);

	else if ((balance_factor > 1) && (comprare_keys(key, node->left_son->key) > 0))
	{
		node->left_son = rotation_left(node->left_son);
		return rotation_right(node);
	}

	else if ((balance_factor < -1) && (comprare_keys(key, node->right_son->key) > 0))
		return rotation_left(node);

	else if ((balance_factor < -1) && (comprare_keys(key, node->right_son->key) < 0))
	{
		node->right_son = rotation_right(node->right_son);
		return rotation_left(node);
	}

	return node;
}

/**
 * @brief vloží prvek do AVL stromu a aktualizuje kořen
 *
 * @param node - ukazatel na kořen stromu
 * @param key - vkládaný klíč
 * @param data - data přiřazená ke klíči
 */
void avl_insertion(AVLNode** node, const char* key, void* data)
{
	*node = avl_insertion_recrusive(*node, key, data);
}

/**
 * @brief rekurzivně uvolní všechny uzly a jejich klíče v post-order průchodu
 *
 * @param node - aktuální uzel podstromu
 */
void avl_disposal(AVLNode* node)
{
	if (node != NULL)
	{
		avl_disposal(node->left_son);
		avl_disposal(node->right_son);
		free(node->key);
		free(node);
	}
}

/**
 * @brief vyhledá uzel podle klíče v AVL stromu
 *
 * @param node - aktuální uzel podstromu
 * @param key - hledaný klíč
 * @return AVLNode* - nalezený uzel nebo NULL
 */
AVLNode* avl_search(AVLNode* node, const char* key)
{
	if (node == NULL)
		return NULL;

	if (comprare_keys(key, node->key) == 0)
		return node;

	else if (comprare_keys(key, node->key) > 0)
		return avl_search(node->right_son, key);

	else
		return avl_search(node->left_son, key);
}
