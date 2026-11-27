/**
 * Project: IFJ26
 * @file symtable.c
 * @brief implementace operací nad tabulkou symbolů
 *
 * @author xsmekav00, Veronika Smékalová
 */

#include "symtable.h" // rozhraní tabulky symbolů

#include <stdbool.h> // datový typ bool
#include <stdio.h>	 // standardní vstup/výstup
#include <stdlib.h>	 // dynamická alokace paměti (malloc, free)
#include <string.h>	 // práce s řetězci

/**
 * @brief inicializuje prázdnou tabulku symbolů
 *
 * @param node - ukazatel na kořen tabulky symbolů
 */
void table_inicialization(AVLNode** node)
{
	avl_inicialization(node);
}

/**
 * @brief vloží nový symbol do tabulky nebo aktualizuje data existujícího symbolu
 *
 * @param node - ukazatel na kořen tabulky symbolů
 * @param key - název symbolu
 * @param type - datový typ symbolu
 * @param tag - druh symbolu (proměnná, val, funkce)
 * @param line - číslo řádku v kódu
 * @return bool - true při úspěchu, false při chybě alokace
 */
bool table_insertion(AVLNode** node, const char* key, DataType type, SymbolTag tag, int line)
{
	SymbolData* data = malloc(sizeof(SymbolData));

	if (data == NULL)
		return false;

	data->type = type;
	data->tag = tag;
	data->parametrs_types = NULL;
	data->parametrs_count = 0;
	data->line_number = line;
	data->is_inicialized = false;

	AVLNode* already_exists = table_search(*node, key);
	if (already_exists != NULL)
	{
		SymbolData* vintage_data = (SymbolData*)already_exists->data;

		if (vintage_data != NULL)
		{
			free(vintage_data->parametrs_types);
			free(vintage_data);
		}

		already_exists->data = data;
	}

	else
		avl_insertion(node, key, data);

	return true;
}

/**
 * @brief rekurzivně uvolní uzly tabulky symbolů včetně struktury SymbolData a klíčů
 *
 * @param node - aktuální uzel podstromu
 */
void table_free_recrusive(AVLNode* node)
{
	if (node != NULL)
	{
		table_free_recrusive(node->left_son);
		table_free_recrusive(node->right_son);

		if (node->data != NULL)
		{
			SymbolData* data = (SymbolData*)node->data;
			free(data->parametrs_types);
			free(data);
		}

		free(node->key);
		free(node);
	}
}

/**
 * @brief uvolní celou tabulku symbolů a nastaví její kořen na NULL
 *
 * @param node - ukazatel na kořen tabulky symbolů
 */
void table_free(AVLNode** node)
{
	if (node != NULL && *node != NULL)
	{
		table_free_recrusive(*node);
		*node = NULL;
	}
}

/**
 * @brief vyhledá symbol v tabulce symbolů podle klíče
 *
 * @param node - kořen tabulky symbolů
 * @param key - hledaný název symbolu
 * @return AVLNode* - nalezený uzel nebo NULL
 */
AVLNode* table_search(AVLNode* node, const char* key)
{
	return avl_search(node, key);
}
