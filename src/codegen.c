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
} Local;

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
} Generator;

/*
 * Oznámí chybu alokace, výstupu nebo jiný předpoklad generátoru.
 */
static void fail(const char* message)
{
	fprintf(stderr, "codegen: %s\n", message);
	exit(99);
}

/*
 * Vypíše formátovanou instrukci IFJcode26 nebo několik řádků kódu.
 */
static void emit(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int result = vprintf(format, args);
	va_end(args);
	if (result < 0)
	{
		fail("cannot write generated code");
	}
}

/*
 * Vrátí globálně unikátní číslo interního návěští.
 */
static size_t label(Generator* gen)
{
	return gen->next_label++;
}

