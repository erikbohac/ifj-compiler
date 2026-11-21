/**
 * Project: IFJ26
 * @file semantic.c
 *
 * @author xsmekav00, Veronika Smékalová
 */

#include "semantic.h"

#include "ast.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool analyse_expression(AstNode* node, AVLNode** symtable)
{
	if (node == NULL)
		return true;

	switch (node->kind)
	{
	case AST_FUNC_CALL:
		return analyse_expression(node->arguments, symtable);

	case AST_BINARY_OP:
		if (!analyse_expression(node->left, symtable))
			return false;

		return analyse_expression(node->right, symtable);

	case AST_UNARY_OP:
		return analyse_expression(node->left, symtable);

	case AST_LIT:

	case AST_ID:
		return true;

	default:
		return false;
	}
}

static bool analyse_node(AstNode* node, AVLNode** symtable)
{
	if (node == NULL)
		return true;

	switch (node->kind)
	{
	case AST_PROGRAM:
		if (!analyse_node(node->body, symtable))
			return false;

		break;

	case AST_FUNC_DEF:
		if (!analyse_node(node->parameters, symtable))
			return false;

		else if (!analyse_node(node->body, symtable))
			return false;

		break;

	case AST_PARAMETR:
		break;

	case AST_VAR_DEF:
		if (!analyse_expression(node->value, symtable))
			return false;
		break;

	case AST_ASSIGN:
		if (!analyse_expression(node->value, symtable))
			return false;
		break;

	case AST_IF:
		if (!analyse_expression(node->condition, symtable))
			return false;

		else if (!analyse_node(node->body, symtable))
			return false;

		else if (!analyse_node(node->else_body, symtable))
			return false;

		break;

	case AST_WHILE:
		if (!analyse_expression(node->condition, symtable))
			return false;

		else if (!analyse_node(node->body, symtable))
			return false;

		break;

	case AST_RETURN:
		if (!analyse_expression(node->value, symtable))
			return false;

		break;

	case AST_FUNC_CALL:
		if (!analyse_expression(node->arguments, symtable))
			return false;

		break;

	case AST_BINARY_OP:
		if (!analyse_expression(node, symtable))
			return false;

		break;

	case AST_UNARY_OP:

	case AST_LIT:

	case AST_ID:

	default:
		return false;
	}

	return analyse_node(node->next, symtable);
}

bool semantic_analyze(AstNode* node, AVLNode** symtable)
{
	if (node == NULL || symtable == NULL)
		return false;

	return analyse_node(node, symtable);
}
