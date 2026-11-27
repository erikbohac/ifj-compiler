/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:  expr.c
 * Popis:   Precedencni syntakticka analyza vyrazu
 * Autor:   Nguyen Trung Hieu (xnguyet01)
 */

#include "expr.h"

#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
#include <stdlib.h>

// Symboly precedencni tabulky
typedef enum
{
	S_UMINUS, // unarni -
	S_MULDIV, // * /
	S_ADDSUB, // + -
	S_REL,	  // < <= > >=
	S_EQ,	  // == !=
	S_LPAR,	  // (
	S_RPAR,	  // )
	S_ID,	  // identifikator nebo literal
	S_END,	  // $ (konec vyrazu)
	S_COUNT
} Symbol;

// Akce v tabulce
typedef enum
{
	SHIFT,	// <
	REDUCE, // >
	EQUAL,	// =
	ERROR,	// prazdne policko
} Action;

#define S SHIFT
#define R REDUCE
#define Q EQUAL
#define X ERROR

// Precedencni tabulka: radek = terminal na vrcholu zasobniku, sloupec = vstup
static const Action table[S_COUNT][S_COUNT] = {
	//            u-  */  +-  rel ==  (   )   id  $
	/* u-  */ {S, R, R, R, R, S, R, S, R},
	/* * / */ {S, R, R, R, R, S, R, S, R},
	/* + - */ {S, S, R, R, R, S, R, S, R},
	/* rel */ {S, S, S, X, R, S, R, S, R},
	/* ==  */ {S, S, S, S, X, S, R, S, R},
	/* (   */ {S, S, S, S, S, S, Q, S, X},
	/* )   */ {X, R, R, R, R, X, R, X, R},
	/* id  */ {X, R, R, R, R, X, R, X, R},
	/* $   */ {S, S, S, S, S, S, X, S, X},
};

#undef S
#undef R
#undef Q
#undef X

// Polozka na zasobniku
typedef enum
{
	ITEM_TERM,	  // terminal operator, zavorka, id, $
	ITEM_NONTERM, // E uz zpracovany podvyraz
	ITEM_MARK,	  // znacka < zacatek handle
} ItemKind;

typedef struct
{
	ItemKind kind;
	Symbol sym;	   // druh terminalu
	OpType op;	   // druh operatoru
	AstNode* node; // uzel pro id a pro E
} Item;

#define STACK_MAX 1000

static Item stack[STACK_MAX];
static int top;			  // index vrcholu zasobniku
static bool unit_pending; // nasel se literal ()

// Vlozi polozku na vrchol zasobniku.
static void push(ItemKind kind, Symbol sym, OpType op, AstNode* node)
{
	if (top + 1 >= STACK_MAX)
	{
		exit(99);
	}
	top++;
	stack[top].kind = kind;
	stack[top].sym = sym;
	stack[top].op = op;
	stack[top].node = node;
}

// Vrati index nejvyssiho terminalu na zasobnik
static int top_terminal(void)
{
	int i = top;
	while (stack[i].kind != ITEM_TERM)
	{
		i--;
	}
	return i;
}

// Vlozi znacku < hned za terminal na indexu index.
static void insert_mark(int index)
{
	push(ITEM_MARK, S_END, OP_NONE, NULL);
	for (int i = top; i > index + 1; i--)
	{
		stack[i] = stack[i - 1];
	}
	stack[index + 1].kind = ITEM_MARK;
	stack[index + 1].node = NULL;
}

// Vytvori uzel pro identifikator nebo literal z aktualniho tokenu
static AstNode* make_term(void)
{
	AstNode* node = new_node(AST_LIT);
	if (unit_pending)
	{
		node->data_type = TYPE_UNIT; // literal ()
		return node;
	}
	switch (current.type)
	{
	case TOK_ID:
		node->kind = AST_ID;
		node->name = current.value.str_val;
		break;
	case TOK_LIT_INT:
		node->data_type = TYPE_INT;
		node->int_val = current.value.int_val;
		break;
	case TOK_LIT_DOUBLE:
		node->data_type = TYPE_DOUBLE;
		node->double_val = current.value.double_val;
		break;
	default:
		node->data_type = TYPE_STRING;
		node->str_val = current.value.str_val;
		break;
	}
	return node;
}

// Urci symbol tabulky pro aktualni token
// prev_operand = true, kdyz pred tokenem byl operand (id nebo ')').
static Symbol get_symbol(bool prev_operand, int depth, OpType* op)
{
	*op = OP_NONE;
	if (unit_pending)
	{
		return S_ID;
	}
	switch (current.type)
	{
	case TOK_MINUS:
		if (!prev_operand)
		{
			*op = OP_NEG;
			return S_UMINUS;
		}
		*op = OP_SUB;
		return S_ADDSUB;
	case TOK_PLUS:
		*op = OP_ADD;
		return S_ADDSUB;
	case TOK_MUL:
		*op = OP_MUL;
		return S_MULDIV;
	case TOK_DIV:
		*op = OP_DIV;
		return S_MULDIV;
	case TOK_LT:
		*op = OP_LESSTHAN;
		return S_REL;
	case TOK_GT:
		*op = OP_GREATERTHAN;
		return S_REL;
	case TOK_LE:
		*op = OP_LESSOREQUAL;
		return S_REL;
	case TOK_GE:
		*op = OP_GREATEROREQUAL;
		return S_REL;
	case TOK_EQ:
		*op = OP_EQUAL;
		return S_EQ;
	case TOK_NEQ:
		*op = OP_NOTEQUAL;
		return S_EQ;
	case TOK_LPAREN:
	{
		// podivame se, jestli neni hned ')' -> literal ()
		Token lpar = current;
		next_token();
		if (current.type == TOK_RPAREN)
		{
			unit_pending = true;
			return S_ID;
		}
		lexer_unget(&current);
		current = lpar;
		return S_LPAR;
	}
	case TOK_RPAREN:
		// ')' bez otevrene '(' uz do vyrazu nepatri
		return depth > 0 ? S_RPAR : S_END;
	case TOK_ID:
	case TOK_LIT_INT:
	case TOK_LIT_DOUBLE:
	case TOK_LIT_STRING:
		return S_ID;
	default:
		return S_END;
	}
}

// Redukce: vse nad znackou < nahradi jednim E podle pravidla.
static void reduce(void)
{
	int mark = top;
	while (mark > 0 && stack[mark].kind != ITEM_MARK)
	{
		mark--;
	}
	if (stack[mark].kind != ITEM_MARK)
	{
		syntax_error("chybny vyraz");
	}

	int count = top - mark; // pocet polozek nad znackou
	Item* h = &stack[mark + 1];
	AstNode* result = NULL;
	if (count == 1 && h[0].kind == ITEM_TERM && h[0].sym == S_ID)
	{
		// E -> id
		result = h[0].node;
	}
	else if (count == 3 && h[0].kind == ITEM_TERM && h[0].sym == S_LPAR &&
			 h[1].kind == ITEM_NONTERM && h[2].kind == ITEM_TERM && h[2].sym == S_RPAR)
	{
		// E -> ( E )
		result = h[1].node;
	}
	else if (count == 2 && h[0].kind == ITEM_TERM && h[0].sym == S_UMINUS &&
			 h[1].kind == ITEM_NONTERM)
	{
		// E -> - E
		result = new_node(AST_UNARY_OP);
		result->op = OP_NEG;
		result->left = h[1].node;
	}
	else if (count == 3 && h[0].kind == ITEM_NONTERM && h[1].kind == ITEM_TERM &&
			 h[2].kind == ITEM_NONTERM)
	{
		// E -> E op E
		result = new_node(AST_BINARY_OP);
		result->op = h[1].op;
		result->left = h[0].node;
		result->right = h[2].node;
	}
	else
	{
		syntax_error("chybny vyraz");
	}
	top = mark - 1; // odstranime handle i se znackou
	push(ITEM_NONTERM, S_END, OP_NONE, result);
}

AstNode* parse_expression(void)
{
	top = -1;
	unit_pending = false;
	push(ITEM_TERM, S_END, OP_NONE, NULL); // $ na dne zasobniku
	bool prev_operand = false;			   // byl posledni symbol operand?
	int depth = 0;						   // pocet otevrenych zavorek

	while (true)
	{
		OpType op;
		Symbol b = get_symbol(prev_operand, depth, &op);
		int a_index = top_terminal();
		Symbol a = stack[a_index].sym;
		// hotovo: na zasobniku je $ E a na vstupu konec vyrazu
		if (a == S_END && b == S_END)
		{
			if (top == 1 && stack[1].kind == ITEM_NONTERM)
			{
				return stack[1].node;
			}
			syntax_error("chybny vyraz");
		}

		Action action = table[a][b];
		if (action == REDUCE)
		{
			reduce();
			continue;
		}
		if (action == ERROR)
		{
			syntax_error("chybny vyraz");
		}
		if (action == SHIFT)
		{
			insert_mark(a_index);
		}

		// SHIFT i EQUAL: vstupni symbol vlozime na zasobnik
		AstNode* node = NULL;
		if (b == S_ID)
		{
			node = make_term();
		}
		push(ITEM_TERM, b, op, node);

		if (b == S_LPAR)
		{
			depth++;
		}
		if (b == S_RPAR)
		{
			depth--;
		}
		prev_operand = (b == S_ID || b == S_RPAR);
		unit_pending = false;
		next_token();
	}
}