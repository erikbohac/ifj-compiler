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

/*
 * Vypíše řetězcový operand a nahradí rezervované a netisknutelné
 * znaky jejich číselnými escape sekvencemi.
 */
static void emit_string(const char* str)
{
	if (!str)
	{
		fail("missing string literal");
	}

	emit("string@");

	for (const unsigned char* byte = (const unsigned char*)str; *byte; ++byte)
	{
		if (*byte <= 32 || *byte == '#' || *byte == '\\' || *byte >= 127)
		{
			emit("\\%03u", (unsigned)*byte);
		}
		else
		{
			emit("%c", *byte);
		}
	}
}

/*
 * Vloží hodnotu literálu na datový zásobník podle jeho datového typu.
 * Každý výraz zanechá na zásobníku právě jednu hodnotu, včetně typu Unit.
 */
static DataType literal(const AstNode* node)
{
	switch (node->data_type)
	{
		case TYPE_INT:
			emit("PUSHS int@%lld\n", node->int_val);
			break;
        case TYPE_DOUBLE:
			emit("PUSHS float@%a\n", node->double_val);
			break;
        case TYPE_STRING:
			emit("PUSHS ");
			emit_string(node->str_val);
			emit("\n");
			break;
        case TYPE_BOOL:
			emit("PUSHS bool@%s\n", node->int_val ? "true" : "false");
			break;
        case TYPE_UNIT:
			emit("PUSHS nil@nil\n");
			break;
		default:
			fail("unknown literal type");
			break;
	}

	return node->data_type;
}

/*
 * Provede jedinou povolenou implicitní číselnou konverzi
 * hodnoty na datovém zásobníku, a to z typu Int na Double.
 */
static void convert(DataType from, DataType to)
{
	if (from == to)
	{
		return;
	}

	if (from == TYPE_INT && to == TYPE_DOUBLE)
	{
		emit("INT2FLOATS\n");
	}
	else
	{
		fail("incompatible types in semantically checked AST");
	}
}

/*
 * Normalizuje celočíselný výsledek na zásobníku do rozsahu
 * 32bitového celého čísla se znaménkem podle pravidel jazyka Scala.
 * Výsledek sčítání, odčítání, násobení a dělení dvou hodnot typu Int
 * se před normalizací vejde do 64bitového celého čísla se znaménkem
 * cílového jazyka.
 */
static void wrap_int(Generator* gen)
{
	size_t id = label(gen);
	emit("POPS LF@%%a\n");
	emit("IDIV LF@%%b LF@%%a int@4294967296\n");
	emit("MUL LF@%%b LF@%%b int@4294967296\n");
	emit("SUB LF@%%a LF@%%a LF@%%b\n");
	emit("GT LF@%%flag LF@%%a int@2147483647\n");
	emit("JUMPIFEQ $L%zu_low LF@%%flag bool@false\n", id);
	emit("SUB LF@%%a LF@%%a int@4294967296\n");
	emit("LABEL $L%zu_low\n", id);
	emit("LT LF@%%flag LF@%%a int@-2147483648\n");
	emit("JUMPIFEQ $L%zu_done LF@%%flag bool@false\n", id);
	emit("ADD LF@%%a LF@%%a int@4294967296\n");
	emit("LABEL $L%zu_done\nPUSHS LF@%%a\n", id);
}

static DataType expression(Generator* gen, const AstNode* node);

/*
 * Vyhodnotí stanovený počet argumentů vestavěné funkce v pořadí,
 * ve kterém jsou uvedeny ve zdrojovém kódu. V případě potřeby
 * provede implicitní konverzi na očekávaný datový typ.
 */
static void builtin_args(Generator* gen, const AstNode* args, size_t count, const DataType* expected, DataType* actual)
{
	for (size_t i = 0; i < count; ++i)
	{
		if (!args)
		{
			fail("missing builtin argument");
		}

		DataType type = expression(gen, args);
		if (expected && expected[i] != TYPE_NONE)
		{
			convert(type, expected[i]);
			type = expected[i];
        }
        if (actual)
		{
            actual[i] = type;
		}
        args = args->next;
    }

	if (args)
	{
		fail("too many builtin arguments");
	}
}

/*
 * Generuje instrukce vestavěných funkcí přímo v místě jejich volání.
 * Dočasné lokální proměnné se používají až po vyhodnocení argumentů,
 * aby vnořená volání funkcí nemohla přepsat hodnoty operandů.
 */
static DataType builtin(Generator* gen, const AstNode* node)
{
	const char* name = node->name;
	const AstNode* args = node->arguments;

	if (!strcmp(name, "print") || !strcmp(name, "toStr"))
    {
		DataType type;
		builtin_args(gen, args, 1, NULL, &type);

		if (!strcmp(name, "print"))
		{
			emit("POPS LF@%%a\n");
			emit(type == TYPE_UNIT ? "WRITE string@()\n" : "WRITE LF@%%a\n");
			emit("PUSHS nil@nil\n");
			return TYPE_UNIT;
		}

		switch (type)
		{
			case TYPE_INT:
				emit("INT2STRS\n");
				break;
			case TYPE_DOUBLE:
				emit("FLOAT2STRS\n");
				break;
			case TYPE_STRING:
				break;
			case TYPE_UNIT:
				emit("POPS LF@%%a\nPUSHS string@()\n");
				break;
			case TYPE_BOOL:
            {
				size_t id = label(gen);
				emit("POPS LF@%%a\nJUMPIFEQ $L%zu_false LF@%%a bool@false\n", id);
				emit("PUSHS string@true\nJUMP $L%zu_done\nLABEL $L%zu_false\n", id, id);
				emit("PUSHS string@false\nLABEL $L%zu_done\n", id);
				break;
            }
			default:
				fail("unresolved toStr argument");
				break;
		}

		return TYPE_STRING;
	}

	if (!strcmp(name, "readInt") || !strcmp(name, "readDouble") || !strcmp(name, "readLine"))
	{
		builtin_args(gen, args, 0, NULL, NULL);
		DataType type = !strcmp(name, "readInt") ? TYPE_INT : !strcmp(name, "readDouble") ? TYPE_DOUBLE : TYPE_STRING;
		const char* kind = type == TYPE_INT ? "int" : type == TYPE_DOUBLE ? "float" : "string";
		const char* fallback = type == TYPE_INT ? "int@0" : type == TYPE_DOUBLE ? "float@0x0p+0" : "string@";
		size_t id = label(gen);

		emit("READ LF@%%a %s\nJUMPIFEQ $L%zu_default LF@%%a nil@nil\n", kind, id);

		if (type == TYPE_INT)
		{
			emit("LT LF@%%flag LF@%%a int@-2147483648\nJUMPIFEQ $L%zu_default LF@%%flag bool@true\n", id);
			emit("GT LF@%%flag LF@%%a int@2147483647\nJUMPIFEQ $L%zu_default LF@%%flag bool@true\n", id);
		}
		emit("JUMP $L%zu_done\nLABEL $L%zu_default\nMOVE LF@%%a %s\n", id, id, fallback);
		emit("LABEL $L%zu_done\nPUSHS LF@%%a\n", id);
		return type;
	}

	if (!strcmp(name, "toInt"))
	{
		builtin_args(gen, args, 1, (DataType[]){TYPE_DOUBLE}, NULL);
		size_t id = label(gen);

		emit("POPS LF@%%a\nEQ LF@%%flag LF@%%a LF@%%a\n");
		emit("JUMPIFEQ $L%zu_nan LF@%%flag bool@false\n", id);
		emit("GT LF@%%flag LF@%%a float@0x1.fffffffcp+30\n");
		emit("JUMPIFEQ $L%zu_max LF@%%flag bool@true\n", id);
		emit("LT LF@%%flag LF@%%a float@-0x1p+31\n");
		emit("JUMPIFEQ $L%zu_min LF@%%flag bool@true\n", id);
		emit("FLOAT2INT LF@%%a LF@%%a\nJUMP $L%zu_done\n", id);
		emit("LABEL $L%zu_nan\nMOVE LF@%%a int@0\nJUMP $L%zu_done\n", id, id);
		emit("LABEL $L%zu_max\nMOVE LF@%%a int@2147483647\nJUMP $L%zu_done\n", id, id);
		emit("LABEL $L%zu_min\nMOVE LF@%%a int@-2147483648\n", id);
		emit("LABEL $L%zu_done\nPUSHS LF@%%a\n", id);
		return TYPE_INT;
    }

	if (!strcmp(name, "strLength"))
	{
		builtin_args(gen, args, 1, (DataType[]){TYPE_STRING}, NULL);
		emit("POPS LF@%%a\nSTRLEN LF@%%a LF@%%a\nPUSHS LF@%%a\n");
		return TYPE_INT;
	}

	if (!strcmp(name, "chr"))
	{
		builtin_args(gen, args, 1, (DataType[]){TYPE_INT}, NULL);
		emit("INT2CHARS\n");
		return TYPE_STRING;
	}

	if (!strcmp(name, "ord"))
	{
		builtin_args(gen, args, 2, (DataType[]){TYPE_STRING, TYPE_INT}, NULL);
		size_t id = label(gen);
		emit("POPS LF@%%b\nPOPS LF@%%a\nSTRLEN LF@%%len LF@%%a\n");
		emit("LT LF@%%flag LF@%%b int@0\nJUMPIFEQ $L%zu_bad LF@%%flag bool@true\n", id);
		emit("LT LF@%%flag LF@%%b LF@%%len\nJUMPIFEQ $L%zu_bad LF@%%flag bool@false\n", id);
		emit("STRI2INT LF@%%a LF@%%a LF@%%b\nJUMP $L%zu_done\nLABEL $L%zu_bad\n", id, id);
		emit("MOVE LF@%%a int@-1\nLABEL $L%zu_done\nPUSHS LF@%%a\n", id);
		return TYPE_INT;
	}

	if (!strcmp(name, "subStr"))
	{
		builtin_args(gen, args, 3, (DataType[]){TYPE_STRING, TYPE_INT, TYPE_INT}, NULL);
		size_t id = label(gen);
		emit("POPS LF@%%c\nPOPS LF@%%b\nPOPS LF@%%a\nSTRLEN LF@%%len LF@%%a\n");
		emit("LT LF@%%flag LF@%%b int@0\nJUMPIFEQ $L%zu_bad LF@%%flag bool@true\n", id);
		emit("LT LF@%%flag LF@%%c int@0\nJUMPIFEQ $L%zu_bad LF@%%flag bool@true\n", id);
		emit("GT LF@%%flag LF@%%b LF@%%c\nJUMPIFEQ $L%zu_bad LF@%%flag bool@true\n", id);
		emit("LT LF@%%flag LF@%%b LF@%%len\nJUMPIFEQ $L%zu_bad LF@%%flag bool@false\n", id);
		emit("GT LF@%%flag LF@%%c LF@%%len\nJUMPIFEQ $L%zu_bad LF@%%flag bool@true\n", id);
		emit("MOVE LF@%%result string@\nLABEL $L%zu_loop\n", id);
		emit("JUMPIFEQ $L%zu_done LF@%%b LF@%%c\n", id);
		emit("GETCHAR LF@%%ch LF@%%a LF@%%b\nCONCAT LF@%%result LF@%%result LF@%%ch\nADD LF@%%b LF@%%b int@1\n");
		emit("JUMP $L%zu_loop\nLABEL $L%zu_bad\nEXIT int@40\nLABEL $L%zu_done\nPUSHS LF@%%result\n", id, id, id);
		return TYPE_STRING;
	}

	fail("unknown function");
	return TYPE_NONE;
}

