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
		fail("problem writing generated code");
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
static const AstNode* find_function(const Generator* gen, const char* name)
{
	for (const AstNode* find = gen->program->body; find; find = find->next)
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
static DataType literal(const AstNode* nodes)
{
	switch (nodes->data_type)
	{
		case TYPE_INT:
			emit("PUSHS int@%lld\n", nodes->int_val);
			break;
        case TYPE_DOUBLE:
			emit("PUSHS float@%a\n", nodes->double_val);
			break;
        case TYPE_STRING:
			emit("PUSHS ");
			emit_string(nodes->str_val);
			emit("\n");
			break;
        case TYPE_BOOL:
			emit("PUSHS bool@%s\n", nodes->int_val ? "true" : "false");
			break;
        case TYPE_UNIT:
			emit("PUSHS nil@nil\n");
			break;
		default:
			fail("unknown literal type");
			break;
	}

	return nodes->data_type;
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
		fail("incompatible data types");
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

static DataType expression(Generator* gen, const AstNode* nodes);

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
static DataType builtin(Generator* gen, const AstNode* nodes)
{
	const char* name = nodes->name;
	const AstNode* args = nodes->arguments;

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

/*
 * Generuje volání uživatelsky definované funkce. Argumenty ukládá
 * na datový zásobník v pořadí jejich výskytu ve zdrojovém kódu.
 * Volaná funkce je odebírá v opačném pořadí do vlastního rámce
 * a na zásobník ukládá právě jednu návratovou hodnotu.
 */
static DataType call(Generator* gen, const AstNode* nodes)
{
	const AstNode* func = find_function(gen, nodes->name);

	if (!func)
	{
		return builtin(gen, nodes);
	}

	const AstNode* args = nodes->arguments;
	const AstNode* params = func->parameters;
	while (args && params)
	{
		convert(expression(gen, args), params->data_type);
		args = args->next;
		params = params->next;
	}

	if (args || params || func->data_type == TYPE_NONE)
	{
		fail("unresolved function signature");
	}

	emit("CALL $fn_%s\n", func->name);
	return func->data_type;
}

/*
 * Vyhodnotí operandy binárního výrazu zleva doprava a v případě
 * rozdílných číselných typů provede implicitní konverzi na společný typ.
 * Následně vygeneruje instrukce odpovídající danému operátoru.
 * Celočíselné aritmetické výsledky normalizuje do rozsahu typu Int.
 */
static DataType binary(Generator* gen, const AstNode* nodes)
{
	DataType left_type = expression(gen, nodes->left);
	DataType right_type = expression(gen, nodes->right);
	DataType common = left_type;

	if (left_type == TYPE_INT && right_type == TYPE_DOUBLE)
	{
		emit("POPS LF@%%b\nINT2FLOATS\nPUSHS LF@%%b\n");
		common = TYPE_DOUBLE;
	}
	else if (left_type == TYPE_DOUBLE && right_type == TYPE_INT)
	{
		emit("INT2FLOATS\n");
		common = TYPE_DOUBLE;
	}
    else if (left_type != right_type)
	{
		fail("incompatible binary operands");
	}

	switch (nodes->op)
	{
		case OP_ADD:
			if (common == TYPE_STRING)
			{
				emit("POPS LF@%%b\nPOPS LF@%%a\nCONCAT LF@%%a LF@%%a LF@%%b\nPUSHS LF@%%a\n");
				return TYPE_STRING;
			}
			emit("ADDS\n");
			break;
        case OP_SUB:
			emit("SUBS\n");
			break;
        case OP_MUL:
			emit("MULS\n");
			break;
        case OP_DIV:
			emit(common == TYPE_INT ? "IDIVS\n" : "DIVS\n");
			break;
        case OP_EQUAL: emit("EQS\n");
			return TYPE_BOOL;
        case OP_NOTEQUAL: emit("EQS\nNOTS\n");
			return TYPE_BOOL;
        case OP_LESSTHAN: emit("LTS\n");
			return TYPE_BOOL;
        case OP_GREATERTHAN: emit("GTS\n");
			return TYPE_BOOL;
		case OP_LESSOREQUAL:
		case OP_GREATEROREQUAL:
			emit("POPS LF@%%b\nPOPS LF@%%a\nPUSHS LF@%%a\nPUSHS LF@%%b\n");
			emit(nodes->op == OP_LESSOREQUAL ? "LTS\n" : "GTS\n");
			emit("PUSHS LF@%%a\nPUSHS LF@%%b\nEQS\nORS\n");
			return TYPE_BOOL;
		default:
			fail("unsupported binary operator");
			break;
    }

	if (common == TYPE_INT)
	{
		wrap_int(gen);
	}

	return common;
}

/*
 * Generuje kód výrazu podle druhu uzlu AST a vrací datový typ
 * výsledné hodnoty. U unárního operátoru negace provádí změnu
 * znaménka a u celočíselných hodnot normalizaci do rozsahu typu Int.
 */
static DataType expression(Generator* gen, const AstNode* nodes)
{
	if (!nodes)
	{
		fail("missing expression");
	}

	switch (nodes->kind)
	{
		case AST_LIT:
			return literal(nodes);
		case AST_ID:
		{
			Local var = lookup(gen, nodes->name);
			emit("PUSHS LF@v%zu\n", var.id);
			return var.type;
		}
		case AST_FUNC_CALL:
			return call(gen, nodes);
		case AST_BINARY_OP:
			return binary(gen, nodes);
		case AST_UNARY_OP:
			if (nodes->op != OP_NEG)
			{
				fail("unsupported unary operator");
			}

			DataType type = expression(gen, nodes->left);
			if (type == TYPE_INT)
			{
				emit("PUSHS int@-1\nMULS\n");
				wrap_int(gen);
			}
			else if (type == TYPE_DOUBLE)
			{
				emit("PUSHS float@-0x1p+0\nMULS\n");
			}
			else
			{
				fail("invalid unary operand");
			}

			return type;
		default:
			fail("unexpected expression");
			break;
	}

	return TYPE_NONE;
}

/*
 * Deklaruje všechny lokální proměnné na začátku funkce ve stejném
 * pořadí, v jakém jsou uzly procházeny do hloubky při generování kódu.
 * Tím zabraňuje opakovanému provádění instrukce DEFVAR uvnitř cyklů.
 */
static void declare_locals(const AstNode* nodes, size_t* id)
{
	for (; nodes; nodes = nodes->next)
	{
		if (nodes->kind == AST_VAR_DEF)
		{
			emit("DEFVAR LF@v%zu\n", (*id)++);
		}
		else if (nodes->kind == AST_IF || nodes->kind == AST_WHILE)
		{
			declare_locals(nodes->body, id);

			if (nodes->kind == AST_IF)
			{
				declare_locals(nodes->else_body, id);
			}
		}
	}
}

/*
 * Zpracuje návratovou hodnotu uloženou na datovém zásobníku.
 * Pokud má funkce návratový typ Unit, nahradí hodnotu za nil.
 * V opačném případě provede případnou konverzi na očekávaný typ.
 * Nakonec vygeneruje skok na konec funkce.
 */
static void finish_return(Generator* gen, DataType actual)
{
	DataType expected = gen->function->data_type;

	if (expected == TYPE_UNIT)
	{
		emit("POPS LF@%%a\nPUSHS nil@nil\n");
	}
	else
	{
		convert(actual, expected);
	}

	emit("JUMP $ret_%s\n", gen->function->name);
}

static void block(Generator* gen, const AstNode* nodes, bool function_body);

/*
 * Generuje kód jednotlivých příkazů podle druhu uzlu AST.
 * Pouze poslední výraz těla funkce tvoří implicitní návratovou
 * hodnotu. Výsledky výrazů ve vnořených blocích jsou zahozeny.
 */
static void statement(Generator* gen, const AstNode* nodes, bool last)
{
	switch (nodes->kind)
	{
		case AST_VAR_DEF:
		{
			size_t id = gen->next_var++;
			DataType actual = expression(gen, nodes->value);
			DataType t = nodes->data_type == TYPE_NONE ? actual : nodes->data_type;
			convert(actual, t);
			emit("POPS LF@v%zu\n", id);

			/*
			 * Vazba proměnné vzniká až po vyhodnocení inicializačního
			 * výrazu, aby bylo správně zajištěno zastínění proměnných.
			 */
			bind(gen, nodes->name, id, t);
			break;
		}
		case AST_ASSIGN:
		{
			Local var = lookup(gen, nodes->name);
			convert(expression(gen, nodes->value), var.type);
			emit("POPS LF@v%zu\n", var.id);
			break;
		}
		case AST_IF:
		{
			size_t id = label(gen);

            if (expression(gen, nodes->condition) != TYPE_BOOL)
			{
				fail("non-boolean if condition");
			}

			emit("PUSHS bool@false\nJUMPIFEQS $L%zu_else\n", id);
			block(gen, nodes->body, false);
			emit("JUMP $L%zu_end\nLABEL $L%zu_else\n", id, id);
			block(gen, nodes->else_body, false);
			emit("LABEL $L%zu_end\n", id);
			break;
		}
		case AST_WHILE:
		{
			size_t id = label(gen);
			emit("LABEL $L%zu_loop\n", id);

			if (expression(gen, nodes->condition) != TYPE_BOOL)
			{
				fail("non-Boolean while condition");
			}

			emit("PUSHS bool@false\nJUMPIFEQS $L%zu_end\n", id);
			block(gen, nodes->body, false);
			emit("JUMP $L%zu_loop\nLABEL $L%zu_end\n", id, id);
			break;
		}
		case AST_RETURN:
			if (nodes->value)
			{
				finish_return(gen, expression(gen, nodes->value));
			}
			else
			{
				emit("PUSHS nil@nil\n");
				finish_return(gen, TYPE_UNIT);
			}
			return;
		case AST_FUNC_CALL:
		case AST_BINARY_OP:
		case AST_UNARY_OP:
		case AST_LIT:
		case AST_ID:
		{
			DataType type = expression(gen, nodes);
			if (last)
			{
				finish_return(gen, type);
			}
			else
			{
				emit("POPS LF@%%a\n");
			}
			return;
		}
		default:
			fail("unexpected statement");
			break;
	}

	/*
	 * Pokud je posledním příkazem funkce s návratovým typem jiným
	 * než Unit podmínka if, musí všechny její možné větve obsahovat
	 * explicitní návrat. Není tedy potřeba vytvářet návratovou hodnotu.
	 */
	if (last && gen->function->data_type == TYPE_UNIT)
	{
		emit("PUSHS nil@nil\nJUMP $ret_%s\n", gen->function->name);
	}
}

/*
 * Generuje kód zřetězeného seznamu příkazů s vlastním rozsahem
 * platnosti. Po opuštění bloku obnoví původní počet vazeb, čímž
 * odstraní vazby lokálních proměnných deklarovaných uvnitř bloku.
 */
static void block(Generator* gen, const AstNode* nodes, bool function_body)
{
	size_t saved = gen->count;

	for (; nodes; nodes = nodes->next)
	{
		statement(gen, nodes, function_body && !nodes->next);
	}

	gen->count = saved;
}

/*
 * Generuje kód uživatelsky definované funkce. Pro každé volání
 * vytváří nový lokální rámec, čímž umožňuje i rekurzivní volání.
 * Deklaruje parametry a lokální proměnné, načte argumenty
 * ze zásobníku a vygeneruje tělo funkce.
 */
static void function(Generator* gen, const AstNode* func)
{
	if (func->data_type == TYPE_NONE)
	{
		fail("invalid function return type");
	}

	gen->function = func;
	gen->count = 0;
	gen->next_var = 0;

	emit("LABEL $fn_%s\nCREATEFRAME\nPUSHFRAME\n", func->name);
	emit("DEFVAR LF@%%a\nDEFVAR LF@%%b\nDEFVAR LF@%%c\nDEFVAR LF@%%len\nDEFVAR LF@%%flag\nDEFVAR LF@%%ch\nDEFVAR LF@%%result\n");

	for (const AstNode* params = func->parameters; params; params = params->next)
	{
		size_t id = gen->next_var++;
		emit("DEFVAR LF@v%zu\n", id);
		bind(gen, params->name, id, params->data_type);
	}

	for (size_t i = gen->next_var; i > 0; --i)
	{
		emit("POPS LF@v%zu\n", i - 1);
	}

	size_t end_id = gen->next_var;
	declare_locals(func->body, &end_id);
	block(gen, func->body, true);

	if (gen->next_var != end_id)
	{
		fail("invalid variable declaration");
	}

	/*
	 * U správně vytvořené funkce s návratovým typem jiným než Unit
	 * nelze dosáhnout konce těla bez vrácení návratové hodnoty.
	 */
	if (func->data_type == TYPE_UNIT)
	{
		emit("PUSHS nil@nil\n");
	}
	else
	{
		emit("EXIT int@40\n");
	}

	emit("LABEL $ret_%s\nPOPFRAME\nRETURN\n", func->name);
	gen->count = 0;
}

