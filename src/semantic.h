/**
 * Implementace překladače imperativního jazyka IFJ26
 * @file semantic.h
 * @brief rozhraní sémantické analýzy AST
 *
 * @author Veronika Smékalová (xsmekav00)
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"	  //abstraktní syntaktický strom
#include "symtable.h" //tabulka symbolů

/**
 * @name semantic_analyze
 * @brief provede sémantickou analýzu aka kontrolu pogramu reprezentovaného AST
 *
 * ověřuje existenci a jedinečnost symbolů, kompaktibilitu typů, správnost
 * argumentů fcií, práci s proměnnými a návratové hodnoty. během toho doplňuje
 * do uzlů AST odvozené info (datový typ a príznak mutability)
 *
 * @param node - kořenový uzel AST
 * @param symtable -ukazatel na kořen tabulky symbolů
 * @return int - při úspěchu návrotový kód 0, jinak jiný příslušný chybový kód
 */
int semantic_analyze(AstNode* node, AVLNode** symtable);

#endif