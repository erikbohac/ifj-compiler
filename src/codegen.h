/*
 * Projekt:	Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:	codegen.h
 * Popis:	Rozhraní generátoru cílového kodu IFJcode26
 *
 * Autor:	Erik Boháč (xbohace00)
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

/*
 * Vypíše vygenerovaný IFJcode26 na stdout.
 * Očekává sémanticky platný AST, včetně vyřešených návratových typů pro funkce.
 * Vnitřní chyby končí kódem 99.
 */
void codegen_generate(const AstNode* program);

#endif

