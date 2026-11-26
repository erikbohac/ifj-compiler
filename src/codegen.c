/*
 * Projekt:	Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:	codegen.h
 * Popis:	Rozhraní generátoru cílového kodu IFJcode26
 *
 * Autor:	Erik Boháč (xbohace00)
 */

#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Jeden záznam pro každou lokální proměnnou v aktuálním rozsahu platnosti.
 * Pole slouží jako zásobník lexikálních vazeb.
 */
typedef struct
{
	const char* name;
	size_t id;
	DataType type;
}
Local;

/*
 * Kontext generátoru kódu obsahující AST programu, aktuální funkci,
 * zásobník lokálních proměnných a čítače pro vytváření unikátních
 * identifikátorů proměnných a návěští.
 */
typedef struct
{
	const AstNode* program;
	const AstNode* function;
	Local* locals;
	size_t count;
	size_t capacity;
	size_t next_var;
	size_t next_label;
}
Generator;

