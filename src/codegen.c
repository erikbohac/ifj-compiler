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

/*
 * Vyhledá aktuálně viditelnou deklaraci proměnné, počínaje
 * nejvnitřnějším rozsahem platnosti.
 */
static Local lookup(const Generator* gen, const char* name)
{
	for (size_t i = gen->count; i > 0; --i)
	{
		if (strcmp(gen->locals[i - 1].name, name) == 0)
		{
			return gen->locals[i - 1];
		}
	}

	fail("unresolved local variable");
	return (Local){0};
}

/*
 * Vyhledá uživatelsky definovanou funkci podle názvu.
 * Podporuje také volání funkcí definovaných později v programu.
 */
static const AstNode* find_function(const Generator* g, const char* name)
{
	for (const AstNode* find = g->program->body; find; find = find->next)
	{
		if (strcmp(find->name, name) == 0)
		{
			return find;
		}
	}

    return NULL;
}

/*
 * Přidá vazbu lokální proměnné do aktuálního rozsahu platnosti.
 * Při opuštění bloku se vazba odstraní obnovením původní hodnoty count.
 * V případě potřeby zvětší kapacitu zásobníku lokálních proměnných.
 */
static void bind(Generator* gen, const char* name, size_t id, DataType type)
{
	if (!name || type == TYPE_NONE)
	{
		fail("unresolved local declaration");
	}

    if (gen->count == gen->capacity)
    {
		size_t capacity = gen->capacity ? gen->capacity * 2 : 16;
		Local* new_locals = realloc(gen->locals, capacity * sizeof(*new_locals));

		if (!new_locals)
		{
			fail("out of memory");
		}

		gen->locals = new_locals;
		gen->capacity = capacity;
    }

    gen->locals[gen->count++] = (Local){name, id, type};
}

