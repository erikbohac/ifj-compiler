/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:  parser.c
 * Popis:   Syntakticka analyza (rekurzivni sestup), vytvari AST
 * Autor:   Nguyen Trung Hieu (xnguyet01)
 */

#include "parser.h"

#include "ast.h"
#include "lexer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token current;

void next_token(void)
{
	int result = lexer_next(&current);
	if (result != 0)
	{
		exit(result);
	}
}

void syntax_error(const char* msg)
{
	fprintf(stderr, "Syntakticka chyba na radku %d: %s\n", current.line, msg);
	exit(2);
}

AstNode* new_node(AstKind kind)
{
	AstNode* node = ast_new_node(kind, current.line);
	if (node == NULL)
	{
		exit(99);
	}
	return node;
}

// Zkontroluje typ aktualniho tokenu a posune se dal.
static void expect(TokenType type, const char* msg)
{
	if (current.type != type)
	{
		syntax_error(msg);
	}
	next_token();
}

// Ocekava identifikator, vrati jeho retezec a posune se dal.
static char* expect_id(const char* msg)
{
	if (current.type != TOK_ID)
	{
		syntax_error(msg);
	}
	char* id = current.value.str_val;
	next_token();
	return id;
}

// Preskoci prazdne radky.
static void skip_newlines(void)
{
	while (current.type == TOK_NEWLINE)
	{
		next_token();
	}
}

// Pripoji uzel na konec zretezeneho seznamu pres ->next
static void append_node(AstNode** head, AstNode** tail, AstNode* node)
{
	if (*head == NULL)
	{
		*head = node;
		*tail = node;
	}
	else
	{
		(*tail)->next = node;
		*tail = node;
	}
}

// <typ> -> Int | Double | String | Unit
static Datatype parse_type(void)
{
	Datatype type;
	switch (current.type)
	{
	case TOK_KW_INT:
		type = TYPE_INT;
		break;
	case TOK_KW_DOUBLE:
		type = TYPE_DOUBLE;
		break;
	case TOK_KW_STRING:
		type = TYPE_STRING;
		break;
	case TOK_KW_UNIT:
		type = TYPE_UNIT;
		break;
	default:
		syntax_error("ocekavan datovy typ (Int, Double, String, Unit)");
		return TYPE_NONE;
	}
	next_token();
	return type;
}

// Docasna funkce pro vyrazy (termy a literal ())
static AstNode* parse_expression(void)
{
	AstNode* node = NULL;
	if (current.type == TOK_ID)
	{
		node = new_node(AST_ID);
		node->name = current.value.str_val;
		next_token();
	}
	else if (current.type == TOK_LIT_INT)
	{
		node = new_node(AST_LIT);
		node->data_type = TYPE_INT;
		node->int_val = current.value.int_val;
		next_token();
	}
	else if (current.type == TOK_LIT_DOUBLE)
	{
		node = new_node(AST_LIT);
		node->data_type = TYPE_DOUBLE;
		node->double_val = current.value.double_val;
		next_token();
	}
	else if (current.type == TOK_LIT_STRING)
	{
		node = new_node(AST_LIT);
		node->data_type = TYPE_STRING;
		node->str_val = current.value.str_val;
		next_token();
	}
	else if (current.type == TOK_LPAREN)
	{
		next_token();
		if (current.type == TOK_RPAREN)
		{
			node = new_node(AST_LIT);
			node->data_type = TYPE_UNIT;
			next_token();
		}
		else
		{
			syntax_error("nepodporovany vyraz (ceka na precedencni analyzu)");
		}
	}
	else
	{
		syntax_error("ocekavan vyraz");
	}
	return node;
}

// Rozhodne, zda nasleduje volani funkce nebo vyraz
static AstNode* parse_expr_or_call(void)
{
	if (current.type == TOK_ID)
	{
		Token id_tok = current;
		next_token();
		if (current.type == TOK_LPAREN)
		{
			AstNode* call = new_node(AST_FUNC_CALL);
			call->name = id_tok.value.str_val;
			next_token();

			AstNode *args_head = NULL, *args_tail = NULL;
			if (current.type != TOK_RPAREN)
			{
				while (true)
				{
					AstNode* arg = parse_expression();
					append_node(&args_head, &args_tail, arg);

					if (current.type == TOK_COMMA)
					{
						next_token();
						if (current.type == TOK_RPAREN && current.newline_before)
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			}
			expect(TOK_RPAREN, "ocekavana ')' za argumenty funkce");
			call->arguments = args_head;
			return call;
		}
		else
		{
			lexer_unget(&current);
			current = id_tok;
		}
	}
	return parse_expression();
}

// <param_list> -> eps | ID : <typ> (, ID : <typ>)* [,]
static AstNode* parse_params(void)
{
	AstNode *head = NULL, *tail = NULL;
	if (current.type == TOK_RPAREN)
	{
		return NULL;
	}

	while (true)
	{
		AstNode* param = new_node(AST_PARAMETR);
		param->name = expect_id("ocekavano jmeno parametru");
		expect(TOK_COLON, "ocekavana ':' za jmenem parametru");
		param->data_type = parse_type();
		append_node(&head, &tail, param);

		if (current.type == TOK_COMMA)
		{
			next_token();
			if (current.type == TOK_RPAREN && current.newline_before)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	return head;
}

// <funkce> -> [@main] def ID ( <param_list> ) [: <typ>] = (<vyraz_nebo_volani> | NEWLINE <blok>)
static AstNode* parse_function(void)
{
	bool is_main = false;
	if (current.type == TOK_AT_MAIN)
	{
		is_main = true;
		next_token();
		if (current.type == TOK_NEWLINE)
		{
			skip_newlines();
		}
	}

	expect(TOK_KW_DEF, "ocekavana definice funkce 'def'");

	AstNode* func = new_node(AST_FUNC_DEF);
	func->is_main = is_main;
	func->name = expect_id("ocekavano jmeno funkce");

	expect(TOK_LPAREN, "ocekavana '(' v hlavicce funkce");
	func->parameters = parse_params();
	expect(TOK_RPAREN, "ocekavana ')' v hlavicce funkce");

	if (current.type == TOK_COLON)
	{
		next_token();
		func->data_type = parse_type();
	}
	else
	{
		func->data_type = TYPE_NONE;
	}

	expect(TOK_ASSIGN, "ocekavano '=' v definici funkce");

	func->body = parse_expr_or_call();
	if (current.type == TOK_NEWLINE)
	{
		skip_newlines();
	}
	else if (current.type != TOK_EOF)
	{
		syntax_error("ocekavan novy radek za jednoradkovou funkci");
	}

	return func;
}

// <prolog> -> import ifj26 . *
static void parse_prolog(void)
{
	expect(TOK_KW_IMPORT, "chybi prolog");

	if (current.type != TOK_ID || strcmp(current.value.str_val, "ifj26") != 0)
	{
		syntax_error("ocekavano ifj26");
	}
	free(current.value.str_val);
	next_token();

	expect(TOK_DOT, "ocekavana '.'");
	expect(TOK_MUL, "ocekavana '*'");
}

// <program> -> <prolog> <funkce_list> EOF
AstNode* parse_program(void)
{
	next_token();
	skip_newlines();

	AstNode* program = new_node(AST_PROGRAM);
	parse_prolog();

	if (current.type != TOK_NEWLINE && current.type != TOK_EOF)
	{
		syntax_error("ocekavan novy radek za prologem");
	}
	skip_newlines();

	AstNode *funcs_head = NULL, *funcs_tail = NULL;
	while (current.type == TOK_KW_DEF || current.type == TOK_AT_MAIN)
	{
		AstNode* func = parse_function();
		append_node(&funcs_head, &funcs_tail, func);
		skip_newlines();
	}

	program->body = funcs_head;

	if (current.type != TOK_EOF)
	{
		syntax_error("ocekavan konec souboru nebo definice funkce");
	}

	return program;
}