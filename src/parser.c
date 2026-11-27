/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:  parser.c
 * Popis:   Syntakticka analyza (rekurzivni sestup), vytvari AST
 * Autor:   Nguyen Trung Hieu (xnguyet01)
 */

#include "parser.h"

#include "ast.h"
#include "expr.h"
#include "lexer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token current;

// Dopredne deklarace
static AstNode* parse_block(void);
static AstNode* parse_statement(void);

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

// Zkontroluje typ aktualniho tokenu a dal
static void expect(TokenType type, const char* msg)
{
	if (current.type != type)
	{
		syntax_error(msg);
	}
	next_token();
}

// Ocekava identifikator, vrati jeho retezec
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

// Preskoci prazdne radky
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
static DataType parse_type(void)
{
	DataType type;
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

// <term> -> id | int | double | string | () | - int | - double
// Argument volani je v zakladnim zadani jen term (bez FUNEXP).
static AstNode* parse_term(void)
{
	AstNode* node = new_node(AST_LIT);
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
	case TOK_LIT_STRING:
		node->data_type = TYPE_STRING;
		node->str_val = current.value.str_val;
		break;
	case TOK_LPAREN:
		next_token();
		if (current.type != TOK_RPAREN)
		{
			syntax_error("argument funkce muze byt jen term");
		}
		node->data_type = TYPE_UNIT;
		break;
	case TOK_MINUS:
		next_token();
		if (current.type == TOK_LIT_INT)
		{
			node->data_type = TYPE_INT;
			node->int_val = -current.value.int_val;
		}
		else if (current.type == TOK_LIT_DOUBLE)
		{
			node->data_type = TYPE_DOUBLE;
			node->double_val = -current.value.double_val;
		}
		else
		{
			syntax_error("za '-' ocekavano cislo");
		}
		break;
	default:
		syntax_error("argument funkce muze byt jen term");
	}
	next_token();
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
					AstNode* arg = parse_term();
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

// <var_def> -> (val|var) ID [:<typ>]=<vyraz_nebo_volani>
static AstNode* parse_var_def(bool is_mutable)
{
	AstNode* node = new_node(AST_VAR_DEF);
	node->is_mutable = is_mutable;
	next_token();

	node->name = expect_id("ocekavano jmeno promenne");
	if (current.type == TOK_COLON)
	{
		next_token();
		node->data_type = parse_type();
	}
	else
	{
		node->data_type = TYPE_NONE;
	}

	expect(TOK_ASSIGN, "ocekavano '=' pri inicializaci promenne");
	node->value = parse_expr_or_call();
	return node;
}

// <if_stmt> -> if <podminka> then NEWLINE <blok> else NEWLINE <blok>
static AstNode* parse_if(void)
{
	AstNode* node = new_node(AST_IF);
	next_token();

	node->condition = parse_expression();
	expect(TOK_KW_THEN, "ocekavano klicove slovo 'then'");
	expect(TOK_NEWLINE, "ocekavan novy radek za 'then'");
	skip_newlines();

	node->body = parse_block();
	skip_newlines();

	expect(TOK_KW_ELSE, "ocekavana vetev 'else'");
	expect(TOK_NEWLINE, "ocekavan novy radek za 'else'");
	skip_newlines();

	node->else_body = parse_block();
	return node;
}

// <while_stmt> -> while <podminka> do NEWLINE <blok>
static AstNode* parse_while(void)
{
	AstNode* node = new_node(AST_WHILE);
	next_token();

	node->condition = parse_expression();
	expect(TOK_KW_DO, "ocekavano klicove slovo 'do'");
	expect(TOK_NEWLINE, "ocekavan novy radek za 'do'");
	skip_newlines();

	node->body = parse_block();
	return node;
}

// <return_stmt> -> return [<vyraz_nebo_volani>]
static AstNode* parse_return(void)
{
	AstNode* node = new_node(AST_RETURN);
	next_token();

	if (current.type == TOK_NEWLINE || current.type == TOK_SEMICOLON ||
		current.type == TOK_DEDENT || current.type == TOK_EOF)
	{
		node->value = NULL;
	}
	else
	{
		node->value = parse_expr_or_call();
	}
	return node;
}

// Jeden prikaz uvnitr bloku
static AstNode* parse_statement(void)
{
	if (current.type == TOK_KW_VAL)
	{
		return parse_var_def(false);
	}
	if (current.type == TOK_KW_VAR)
	{
		return parse_var_def(true);
	}
	if (current.type == TOK_KW_IF)
	{
		return parse_if();
	}
	if (current.type == TOK_KW_WHILE)
	{
		return parse_while();
	}
	if (current.type == TOK_KW_RETURN)
	{
		return parse_return();
	}

	if (current.type == TOK_ID)
	{
		Token id_tok = current;
		next_token();

		if (current.type == TOK_ASSIGN)
		{
			AstNode* assign = new_node(AST_ASSIGN);
			assign->name = id_tok.value.str_val;
			next_token();
			assign->value = parse_expr_or_call();
			return assign;
		}
		else
		{
			lexer_unget(&current);
			current = id_tok;
			return parse_expr_or_call();
		}
	}
	return parse_expression();
}

// <blok> -> INDENT <prikazy> DEDENT
static AstNode* parse_block(void)
{
	expect(TOK_INDENT, "ocekavano odsazeni noveho bloku");
	skip_newlines();
	if (current.type == TOK_DEDENT)
	{
		syntax_error("blok nesmi byt prazdny");
	}

	AstNode *head = NULL, *tail = NULL;

	while (current.type != TOK_DEDENT && current.type != TOK_EOF)
	{
		AstNode* stmt = parse_statement();
		append_node(&head, &tail, stmt);
		if (current.type == TOK_SEMICOLON)
		{
			next_token();
			if (current.type == TOK_NEWLINE)
			{
				skip_newlines();
			}
		}
		else if (current.type == TOK_NEWLINE)
		{
			skip_newlines();
		}
		else if (current.type == TOK_DEDENT)
		{
			break;
		}
		else if (stmt->kind == AST_IF || stmt->kind == AST_WHILE)
		{
			// if a while konci vlastnim blokem DEDENT, oddelovac neni potreba
		}
		else
		{
			syntax_error("ocekavan konec radku nebo ';' za prikazem");
		}
	}
	expect(TOK_DEDENT, "ocekavano ukonceni odsazeneho bloku");
	return head;
}

// <funkce> -> [@main] def ID (<param_list>) [: <typ>] = (<vyraz_nebo_volani> | NEWLINE <blok>)
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

	if (current.type == TOK_NEWLINE)
	{
		skip_newlines();
		func->body = parse_block();
	}
	else
	{
		func->body = parse_expr_or_call();
		if (current.type == TOK_NEWLINE)
		{
			skip_newlines();
		}
		else if (current.type != TOK_EOF)
		{
			syntax_error("ocekavan novy radek za jednoradkovou funkci");
		}
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

	// Pred prologem smi byt bile znaky, lexer z nich udela INDENT
	bool indented_prolog = false;
	if (current.type == TOK_INDENT)
	{
		indented_prolog = true;
		next_token();
	}

	AstNode* program = new_node(AST_PROGRAM);
	parse_prolog();
	if (current.type != TOK_NEWLINE && current.type != TOK_EOF)
	{
		syntax_error("ocekavan novy radek za prologem");
	}
	skip_newlines();

	if (indented_prolog)
	{
		expect(TOK_DEDENT, "funkce musi zacinat na zacatku radku");
		skip_newlines();
	}

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