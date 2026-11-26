/**
 * Project: IFJ26
 * @file semantic.c
 * @brief lbup blup
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

static int analyse_expression(AstNode* node, AVLNode** symtable)
{
	if (node == NULL)
		return 0;

	switch (node->kind)
	{
		case AST_FUNC_CALL:
		{
			if (node->name == NULL || symtable == NULL || *symtable == NULL)
				return 3;

			AVLNode* found = table_search(*symtable, node->name);
			if (found == NULL || found->data == NULL)
				return 3;
			
			SymbolData* fn = (SymbolData*)found->data;
			if(fn->tag != TAG_FUNCTION || fn->type == TYPE_NONE)
				return 3;
			
			int arg_count = 0;
			for(AstNode* arg = node->arguments; arg != NULL; arg = arg->next){
				int err = analyse_expression(arg, symtable);
				if (err != 0)
					return err;
				
				if(arg_count >= fn->parametrs_count || arg->data_type != fn->parametrs_types[arg_count])
					return 3;

				arg_count++;
			}

			if(arg_count != fn->parametrs_count)
				return 3;

			node->data_type = fn->type;
			return 0;
		}
		case AST_BINARY_OP:
		{
			int err = analyse_expression(node->left, symtable);
			if (err != 0)
				return err;

			err = analyse_expression(node->right, symtable);
			if (err != 0)
				return err;
			
			if(node->left == NULL || node->right == NULL)
				return 3;
			
			DataType lt = node->left->data_type;
			DataType rt = node->right->data_type;

			if (lt == TYPE_NONE || rt == TYPE_NONE)
				return 3;

			switch (node->op){
				case OP_ADD:
				case OP_SUB:
				case OP_MUL:
				case OP_DIV:
					if (lt != rt)
						return 3;

					if (lt != TYPE_INT && lt != TYPE_DOUBLE && !(node->op == OP_ADD && lt == TYPE_STRING))
						return 3;

					node->data_type = lt;
					return 0;
				case OP_EQUAL:
				case OP_NOTEQUAL:
				case OP_LESSTHAN:
				case OP_GREATERTHAN:
				case OP_LESSOREQUAL:
				case OP_GREATEROREQUAL:
					if (lt != rt)
						return 3;
					node->data_type = TYPE_BOOL;
					return 0;
				default:
					return 3;
			}
		}
		case AST_UNARY_OP:
		{
			int err = analyse_expression(node->left, symtable);
			if (err != 0)
				return err;

			if(node->left == NULL || node->left->data_type == TYPE_NONE)
				return 3;

			if(node->op == OP_NEG){
				if (node->left->data_type != TYPE_INT && node->left->data_type != TYPE_DOUBLE)
					return 3;

				node->data_type = node->left->data_type;
				return 0;
			}

			return 3;
		}

		case AST_LIT:
			if(node->data_type == TYPE_NONE)
				return 3;
			return 0;

		case AST_ID:
		{
			if (node->name == NULL || symtable == NULL || *symtable == NULL)
				return 3;
			
			AVLNode* found = table_search(*symtable, node->name);
			if (found == NULL || found->data == NULL)
				return 3;
			
			SymbolData* sym_data = (SymbolData*)found->data;
			if(sym_data->tag == TAG_FUNCTION || !sym_data->is_inicialized)
				return 3;

			node->data_type = sym_data->type;
			node->is_mutable = (sym_data->tag == TAG_VAR);
			return 0;
		}
		default:
			return 3;
	}
}

static int analyse_node(AstNode* node, AVLNode** symtable)
{
	if (node == NULL)
		return 0;

	int err = 0;

	switch (node->kind)
	{
		case AST_PROGRAM:
			err = analyse_node(node->body, symtable);
			if (err != 0)
				return err;

			break;

		case AST_FUNC_DEF:
			err = analyse_node(node->parameters, symtable);
			if (err != 0)
				return err;

			err = analyse_node(node->body, symtable);
			if (err != 0)
				return err;

			break;

		case AST_PARAMETR:
			break;

		case AST_VAR_DEF:
		{
			if(node->name == NULL || symtable == NULL)
				return 3;

			if(table_search(*symtable,node->name) != NULL)
				return 3;

			if(node->value != NULL){
				err = analyse_expression(node->value, symtable);
				if(err != 0)
					return err;

				if(node->data_type == TYPE_NONE)
					node->data_type = node->value->data_type;
				else if (node->value->data_type != node->data_type)
					return 3;
			}

			if(node->data_type == TYPE_NONE)
				return 3;

			SymbolTag tag = node->is_mutable ? TAG_VAR : TAG_VAL;
			if(!table_insertion(symtable, node->name, node->data_type, tag, node->line))
				return 3;

			if(node->value != NULL){
				AVLNode* inserted = table_search(*symtable, node->name);
				if (inserted != NULL && inserted->data != NULL){
					SymbolData* sym_data = (SymbolData*)inserted->data;
				}
			}
			break;
		}
		case AST_ASSIGN:
		{
			if (node->name == NULL || symtable == NULL || *symtable == NULL || node->value == NULL)
				return 3;

			AVLNode* found = table_search(*symtable, node->name);
			if (found == NULL || found->data == NULL)
				return 3;

			SymbolData* sym_data = (SymbolData*)found->data;
			if (sym_data->tag != TAG_VAR)
				return 3;

			err = analyse_expression(node->value, symtable);
			if (err != 0)
				return err;

			if (node->value->data_type == TYPE_NONE || node->value->data_type != sym_data->type)
				return 3;

			sym_data->is_inicialized = true;
			break;
		}

		case AST_IF:
			err = analyse_expression(node->condition, symtable);
			if (err != 0)
				return err;

			err = analyse_node(node->body, symtable);
			if (err != 0)
				return err;

			err = analyse_node(node->else_body, symtable);
			if (err != 0)
				return err;

			break;

		case AST_WHILE:
			err = analyse_expression(node->condition, symtable);
			if (err != 0)
				return err;

			err = analyse_node(node->body, symtable);
			if (err != 0)
				return err;

			break;

		case AST_RETURN:
			err = analyse_expression(node->value, symtable);
			if (err != 0)
				return err;

			break;

		case AST_FUNC_CALL:
			err = analyse_expression(node->arguments, symtable);
			if (err != 0)
				return err;

			break;

		case AST_BINARY_OP:
			int err = analyse_expression(node->left, symtable);
			if(err != 0)
				return err;

			return analyse_expression(node->right, symtable);

		case AST_UNARY_OP:

		case AST_LIT:

		case AST_ID:
		{
			err = analyse_expression(node, symtable);
			if (err != 0)
				return err;

			break;
		}
		default:
			return 3;
	}

	return analyse_node(node->next, symtable);
}

int semantic_analyze(AstNode* node, AVLNode** symtable)
{
	if (node == NULL || symtable == NULL)
		return 3;

	return analyse_node(node, symtable);
}
