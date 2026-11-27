/**
 * Implementace překladače imperativního jazyka IFJ26
 * @file	semantic.c
 * @brief	text
 *
 * @author	Veronika Smékalová (xsmekav00)
 */

#include "semantic.h"

#include "ast.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	DataType expected_return_type;
	bool has_explicit_return_type;
	bool has_return_stmt;
	bool is_main;
} FuncContext;

typedef struct LocalEntry
{
	char* name;
	struct LocalEntry* next;
} LocalEntry;

static bool is_numeric(DataType t)
{
	return t == TYPE_INT || t == TYPE_DOUBLE;
}

static bool is_compatible(DataType from, DataType to)
{
	if (from == TYPE_NONE || to == TYPE_NONE)
		return false;

	if (from == to)
		return true;

	if (from == TYPE_INT && to == TYPE_DOUBLE)
		return true;

	return false;
}

static DataType common_type(DataType a, DataType b)
{
	if (a == TYPE_NONE || b == TYPE_NONE)
		return TYPE_NONE;

	if (a == b)
		return a;

	if (is_numeric(a) && is_numeric(b))
		return TYPE_DOUBLE;

	return TYPE_NONE;
}

static bool record_local_name(LocalEntry** locals, const char* name)
{
	if (name == NULL || locals == NULL)
		return false;

	LocalEntry* entry = malloc(sizeof(LocalEntry));
	if (entry == NULL)
		return false;

	entry->name = malloc(strlen(name) + 1);
	if (entry->name == NULL)
	{
		free(entry);
		return false;
	}

	strcpy(entry->name, name);
	entry->next = *locals;
	*locals = entry;
	return true;
}

static AVLNode* remove_key_recursive(AVLNode* root, const char* key, bool* removed)
{
	if (root == NULL)
		return NULL;

	int cpmprare = strcmp(key, root->key);

	if (cpmprare < 0)
	{
		root->left_son = remove_key_recursive(root->left_son, key, removed);
		return root;
	}

	else if (cpmprare > 0)
	{
		root->right_son = remove_key_recursive(root->right_son, key, removed);
		return root;
	}

	*removed = true;

	if (root->data != NULL)
	{
		SymbolData* d = (SymbolData*)root->data;
		free(d->parametrs_types);
		free(d);
	}

	free(root->key);

	if (root->left_son == NULL)
	{
		AVLNode* right = root->right_son;
		free(root);
		return right;
	}

	if (root->right_son == NULL)
	{
		AVLNode* left = root->left_son;
		free(root);
		return left;
	}

	AVLNode* succ_parent = root;
	AVLNode* succ = root->right_son;

	while (succ->left_son != NULL)
	{
		succ_parent = succ;
		succ = succ->left_son;
	}

	root->key = succ->key;
	root->data = succ->data;

	if (succ_parent == root)
		succ_parent->right_son = succ->right_son;

	else
		succ_parent->left_son = succ->right_son;

	free(succ);
	return root;
}

static void cleanup_locals(AVLNode** symtable, LocalEntry** locals)
{
	if (locals == NULL)
		return;

	LocalEntry* cur = *locals;
	while (cur != NULL)
	{
		LocalEntry* next = cur->next;

		if (symtable != NULL && *symtable != NULL && cur->name != NULL)
		{
			bool removed = false;
			*symtable = remove_key_recursive(*symtable, cur->name, &removed);
		}

		free(cur->name);
		free(cur);
		cur = next;
	}
	*locals = NULL;
}

static bool register_signle_builtin(AVLNode** symtable, const char* name, DataType ret_type,
									int param_count, const DataType* param_types)
{
	if (table_search(*symtable, name) != NULL)
		return true;

	if (!table_insertion(symtable, name, ret_type, TAG_FUNCTION, 0))
		return false;

	AVLNode* node = table_search(*symtable, name);
	if (node == NULL || node->data == NULL)
		return false;

	SymbolData* fn = (SymbolData*)node->data;
	fn->is_inicialized = true;
	fn->parametrs_count = param_count;

	if (param_count > 0)
	{
		fn->parametrs_types = malloc(sizeof(DataType) * (size_t)param_count);

		if (fn->parametrs_types == NULL)
			return false;

		for (int i = 0; i < param_count; i++)
			fn->parametrs_types[i] = param_types[i];
	}

	return true;
}

static int register_builtins(AVLNode** symtable)
{
	static const DataType any_param[] = {TYPE_NONE};
	static const DataType double_param[] = {TYPE_DOUBLE};
	static const DataType int_param[] = {TYPE_INT};
	static const DataType string_param[] = {TYPE_STRING};
	static const DataType substr_param[] = {TYPE_STRING, TYPE_INT, TYPE_INT};
	static const DataType ord_param[] = {TYPE_STRING, TYPE_INT};

	if (!register_signle_builtin(symtable, "print", TYPE_UNIT, 1, any_param))
		return 99;

	if (!register_signle_builtin(symtable, "readLine", TYPE_STRING, 0, NULL))
		return 99;

	if (!register_signle_builtin(symtable, "readInt", TYPE_INT, 0, NULL))
		return 99;

	if (!register_signle_builtin(symtable, "readDouble", TYPE_DOUBLE, 0, NULL))
		return 99;

	if (!register_signle_builtin(symtable, "readBoolean", TYPE_BOOL, 0, NULL))
		return 99;

	if (!register_signle_builtin(symtable, "toStr", TYPE_STRING, 1, any_param))
		return 99;

	if (!register_signle_builtin(symtable, "toInt", TYPE_INT, 1, double_param))
		return 99;

	if (!register_signle_builtin(symtable, "strLength", TYPE_INT, 1, string_param))
		return 99;

	if (!register_signle_builtin(symtable, "substr", TYPE_STRING, 3, substr_param))
		return 99;

	if (!register_signle_builtin(symtable, "ord", TYPE_INT, 2, ord_param))
		return 99;

	if (!register_signle_builtin(symtable, "chr", TYPE_STRING, 1, int_param))
		return 99;

	return 0;
}

static int register_functions(AstNode* node, AVLNode** symtable, int* main_count)
{
	if (node == NULL)
		return 0;

	if (node->kind == AST_PROGRAM)
		return register_functions(node->body, symtable, main_count);

	if (node->kind == AST_FUNC_DEF)
	{
		if (node->name == NULL || symtable == NULL)
			return 99;

		if (table_search(*symtable, node->name) != NULL)
			return 4;

		int param_count = 0;
		for (AstNode* p = node->parameters; p != NULL; p = p->next)
		{
			if (p->data_type == TYPE_NONE)
				return 6;
			param_count++;
		}

		if (node->is_main)
		{
			(*main_count)++;

			if (param_count != 0)
				return 5;

			if (node->data_type != TYPE_UNIT && node->data_type != TYPE_NONE)
				return 6;
		}

		if (!table_insertion(symtable, node->name, node->data_type, TAG_FUNCTION, node->line))
		{
			return 99;
		}

		AVLNode* fn_node = table_search(*symtable, node->name);
		SymbolData* fn_data = (fn_node != NULL) ? (SymbolData*)fn_node->data : NULL;
		if (fn_data == NULL)
			return 99;

		fn_data->is_inicialized = true;
		fn_data->parametrs_count = param_count;

		if (param_count > 0)
		{
			fn_data->parametrs_types = malloc(sizeof(DataType) * (size_t)param_count);
			if (fn_data->parametrs_types == NULL)
				return 99;

			int index = 0;
			for (AstNode* p = node->parameters; p != NULL; p = p->next)
				fn_data->parametrs_types[index++] = p->data_type;
		}
	}
	return register_functions(node->next, symtable, main_count);
}

static int analyse_expression(AstNode* node, AVLNode** symtable, FuncContext* cntx);
static int analyse_node(AstNode* node, AVLNode** symtable, FuncContext* cntx, LocalEntry** locals,
						DataType* out_block_type);

static int analyse_expression(AstNode* node, AVLNode** symtable, FuncContext* cntx)
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
		if (fn->tag != TAG_FUNCTION)
			return 3;

		int arg_count = 0;
		for (AstNode* arg = node->arguments; arg != NULL; arg = arg->next)
		{
			int err = analyse_expression(arg, symtable, cntx);
			if (err != 0)
				return err;

			if (arg_count >= fn->parametrs_count)
				return 5;

			DataType expected_arg = fn->parametrs_types[arg_count];
			if (expected_arg != TYPE_NONE && !is_compatible(arg->data_type, expected_arg))
				return 5;

			arg_count++;
		}

		if (arg_count != fn->parametrs_count)
			return 5;

		node->data_type = (fn->type == TYPE_NONE) ? TYPE_UNIT : fn->type;
		return 0;
	}

	case AST_BINARY_OP:
	{
		if (node->left == NULL || node->right == NULL)
			return 99;

		int err = analyse_expression(node->left, symtable, cntx);
		if (err != 0)
			return err;

		err = analyse_expression(node->right, symtable, cntx);
		if (err != 0)
			return err;

		DataType lt = node->left->data_type;
		DataType rt = node->right->data_type;

		if (lt == TYPE_NONE || rt == TYPE_NONE)
			return 6;

		switch (node->op)
		{
		case OP_ADD:
			if (lt == TYPE_STRING && rt == TYPE_STRING)
			{
				node->data_type = TYPE_STRING;
				return 0;
			}

			if (is_numeric(lt) && is_numeric(rt))
			{
				node->data_type = common_type(lt, rt);
				return 0;
			}

			return 6;
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
			if (is_numeric(lt) && is_numeric(rt))
			{
				node->data_type = common_type(lt, rt);
				return 0;
			}
			return 6;
		case OP_LESSTHAN:
		case OP_GREATERTHAN:
		case OP_LESSOREQUAL:
		case OP_GREATEROREQUAL:
			if ((is_numeric(lt) && is_numeric(rt)) || (lt == TYPE_STRING && rt == TYPE_STRING))
			{
				node->data_type = TYPE_BOOL;
				return 0;
			}

			return 6;
		case OP_EQUAL:
		case OP_NOTEQUAL:
			if ((is_numeric(lt) && is_numeric(rt)) ||
				(lt == rt && (lt == TYPE_STRING || lt == TYPE_BOOL || lt == TYPE_UNIT)))
			{
				node->data_type = TYPE_BOOL;
				return 0;
			}

			return 6;

			/*case OP_AND:
			case OP_OR:
				if (lt == TYPE_BOOL && rt == TYPE_BOOL)
				{
					node->data_type = TYPE_BOOL;
					return 0;
				}

				return 6;*/

		default:
			return 6;
		}
	}

	case AST_UNARY_OP:
	{
		if (node->left == NULL)
			return 99;

		int err = analyse_expression(node->left, symtable, cntx);
		if (err != 0)
			return err;

		if (node->left->data_type == TYPE_NONE)
			return 6;

		if (node->op == OP_NEG)
		{
			if (!is_numeric(node->left->data_type))
				return 6;

			node->data_type = node->left->data_type;
			return 0;
		}

		/*if (node->op == OP_NOT)
		{
			if (node->left->data_type != TYPE_BOOL)
				return 6;

			node->data_type = TYPE_BOOL;
			return 0;
		}*/

		return 6;
	}

	case AST_LIT:
		if (node->data_type == TYPE_NONE)
			return 6;
		return 0;

	case AST_ID:
	{
		if (node->name == NULL || symtable == NULL || *symtable == NULL)
			return 3;

		AVLNode* found = table_search(*symtable, node->name);
		if (found == NULL || found->data == NULL)
			return 3;

		SymbolData* sym_data = (SymbolData*)found->data;
		if (sym_data->tag == TAG_FUNCTION)
			return 3;

		if (!sym_data->is_inicialized)
			return 6;

		node->data_type = sym_data->type;
		node->is_mutable = (sym_data->tag == TAG_VAR);
		return 0;
	}

		/*case AST_IF:
		{
			DataType if_type = TYPE_NONE;
			int err = analyse_node(node, symtable, cntx, NULL, &if_type);
			if (err != 0)
				return err;

			else if (if_type == TYPE_NONE)
				return 6;

			node->data_type = if_type;
			return 0;
		}*/

	default:
		return 6;
	}
}

static int analyse_node(AstNode* node, AVLNode** symtable, FuncContext* cntx, LocalEntry** locals,
						DataType* out_block_type)
{
	if (node == NULL)
		return 0;

	int err = 0;
	DataType stmt_type = TYPE_UNIT;

	switch (node->kind)
	{
	case AST_PROGRAM:
	{
		int main_count = 0;

		err = register_builtins(symtable);
		if (err != 0)
			return err;

		err = register_functions(node->body, symtable, &main_count);
		if (err != 0)
			return err;

		if (main_count == 0)
			return 3;

		if (main_count > 1)
			return 4;

		err = analyse_node(node->body, symtable, NULL, NULL, NULL);
		if (err != 0)
			return err;

		break;
	}

	case AST_FUNC_DEF:
	{
		if (node->name == NULL || symtable == NULL)
			return 99;

		FuncContext fn_cntx = {.expected_return_type = node->data_type,
							   .has_explicit_return_type = (node->data_type != TYPE_NONE),
							   .has_return_stmt = false,
							   .is_main = node->is_main};

		LocalEntry* fn_locals = NULL;

		for (AstNode* p = node->parameters; p != NULL; p = p->next)
		{
			if (p->name == NULL || p->data_type == TYPE_NONE)
			{
				cleanup_locals(symtable, &fn_locals);
				return 99;
			}

			if (table_search(*symtable, p->name) != NULL)
			{
				cleanup_locals(symtable, &fn_locals);
				return 4;
			}

			if (!table_insertion(symtable, p->name, p->data_type, TAG_VAL, p->line))
			{
				cleanup_locals(symtable, &fn_locals);
				return 99;
			}

			if (!record_local_name(&fn_locals, p->name))
			{
				cleanup_locals(symtable, &fn_locals);
				return 99;
			}

			AVLNode* insertedd = table_search(*symtable, p->name);
			if (insertedd != NULL && insertedd->data != NULL)
			{
				SymbolData* sym_data = (SymbolData*)insertedd->data;
				sym_data->is_inicialized = true;
			}
		}

		DataType body_type = TYPE_UNIT;
		err = analyse_node(node->body, symtable, &fn_cntx, &fn_locals, &body_type);
		if (err != 0)
		{
			cleanup_locals(symtable, &fn_locals);
			return err;
		}

		AVLNode* fn_node = table_search(*symtable, node->name);
		SymbolData* fn_data = (fn_node != NULL) ? (SymbolData*)fn_node->data : NULL;
		if (fn_data == NULL)
			return 99;

		if (fn_cntx.has_explicit_return_type)
		{
			if (fn_cntx.expected_return_type != TYPE_UNIT &&
				!is_compatible(body_type, fn_cntx.expected_return_type))
				return 6;

			fn_data->type = fn_cntx.expected_return_type;
		}

		else
		{
			if (fn_cntx.has_return_stmt || body_type == TYPE_NONE)
				return 6;

			if (fn_cntx.is_main && body_type != TYPE_UNIT)
				return 6;

			fn_data->type = body_type;
			node->data_type = body_type;
		}
		break;
	}

	case AST_PARAMETR:
		break;

	case AST_VAR_DEF:
	{
		if (node->name == NULL || symtable == NULL)
			return 99;

		if (table_search(*symtable, node->name) != NULL)
			return 4;

		if (node->value != NULL)
		{
			err = analyse_expression(node->value, symtable, cntx);
			if (err != 0)
				return err;

			if (node->data_type == TYPE_NONE)
				node->data_type = node->value->data_type;

			else if (!is_compatible(node->value->data_type, node->data_type))
				return 6;
		}

		if (node->data_type == TYPE_NONE)
			return 6;

		SymbolTag tag = node->is_mutable ? TAG_VAR : TAG_VAL;
		if (!table_insertion(symtable, node->name, node->data_type, tag, node->line))
			return 99;

		if (locals != NULL && !record_local_name(locals, node->name))
			return 99;

		if (node->value != NULL)
		{
			AVLNode* inserted = table_search(*symtable, node->name);
			if (inserted != NULL && inserted->data != NULL)
			{
				SymbolData* sym_data = (SymbolData*)inserted->data;
				sym_data->is_inicialized = true;
			}
		}
		stmt_type = TYPE_UNIT;
		break;
	}

	case AST_ASSIGN:
	{
		if (node->name == NULL || symtable == NULL || *symtable == NULL || node->value == NULL)
			return 99;

		AVLNode* found = table_search(*symtable, node->name);
		if (found == NULL || found->data == NULL)
			return 3;

		SymbolData* sym_data = (SymbolData*)found->data;
		if (sym_data->tag != TAG_VAR)
			return 7;

		err = analyse_expression(node->value, symtable, cntx);
		if (err != 0)
			return err;

		if (node->value->data_type == TYPE_NONE ||
			!is_compatible(node->value->data_type, sym_data->type))
			return 6;

		sym_data->is_inicialized = true;
		stmt_type = TYPE_UNIT;
		break;
	}

	case AST_IF:
	{
		if (node->condition == NULL)
			return 6;

		err = analyse_expression(node->condition, symtable, cntx);
		if (err != 0)
			return err;

		if (node->condition->data_type != TYPE_BOOL)
			return 6;

		DataType then_type = TYPE_UNIT;
		err = analyse_node(node->body, symtable, cntx, locals, &then_type);
		if (err != 0)
			return err;

		DataType else_type = TYPE_UNIT;
		if (node->else_body != NULL)
		{
			err = analyse_node(node->else_body, symtable, cntx, locals, &else_type);
			if (err != 0)
				return err;

			stmt_type = common_type(then_type, else_type);
		}
		else
		{
			stmt_type = TYPE_UNIT;
		}

		break;
	}

	case AST_WHILE:
	{
		if (node->condition == NULL)
			return 6;

		err = analyse_expression(node->condition, symtable, cntx);
		if (err != 0)
			return err;

		if (node->condition->data_type != TYPE_BOOL)
			return 6;

		err = analyse_node(node->body, symtable, cntx, locals, NULL);
		if (err != 0)
			return err;

		stmt_type = TYPE_UNIT;
		break;
	}

		/*case AST_FOR:
		{
			if (node->name == NULL || symtable == NULL || node->range_start == NULL ||
				node->range_end == NULL)
				return 6;

			if (table_search(*symtable, node->name) != NULL)
				return 4;

			err = analyse_expression(node->range_start, symtable, cntx);
			if (err != 0)
				return err;
			if (node->range_start->data_type != TYPE_INT)
				return 6;

			err = analyse_expression(node->range_end, symtable, cntx);
			if (err != 0)
				return err;
			if (node->range_end->data_type != TYPE_INT)
				return 6;

			if (node->step != NULL)
			{
				err = analyse_expression(node->step, symtable, cntx);
				if (err != 0)
					return err;
				if (node->step->data_type != TYPE_INT)
					return 6;
				if (node->step->kind == AST_LIT && node->step->int_val == 0)
					return 10;
			}

			if (!table_insertion(symtable, node->name, TYPE_INT, TAG_VAL, node->line))
				return 99;

			if (locals != NULL && !record_local_name(locals, node->name))
				return 99;

			AVLNode* iter_node = table_search(*symtable, node->name);
			if (iter_node != NULL && iter_node->data != NULL)
			{
				SymbolData* iterr_data = (SymbolData*)iter_node->data;
				iterr_data->is_inicialized = true;
			}

			if (node->guard != NULL)
			{
				err = analyse_expression(node->guard, symtable, cntx);
				if (err != 0)
					return err;
				if (node->guard->data_type != TYPE_BOOL)
					return 6;
			}

			err = analyse_node(node->body, symtable, cntx, locals, NULL);
			if (err != 0)
				return err;

			stmt_type = TYPE_UNIT;
			break;
		}*/

	case AST_RETURN:
	{
		if (cntx != NULL)
			cntx->has_return_stmt = true;

		if (node->value != NULL)
		{
			err = analyse_expression(node->value, symtable, cntx);
			if (err != 0)
				return err;

			if (cntx != NULL && cntx->has_explicit_return_type &&
				cntx->expected_return_type != TYPE_UNIT)
			{
				if (!is_compatible(node->value->data_type, cntx->expected_return_type))
					return 6;
			}
			stmt_type = node->value->data_type;
		}
		else
		{
			if (cntx == NULL || !cntx->has_explicit_return_type ||
				cntx->expected_return_type != TYPE_UNIT)
				return 10;
			stmt_type = TYPE_UNIT;
		}

		break;
	}

	case AST_FUNC_CALL:
	case AST_BINARY_OP:
	case AST_UNARY_OP:
	case AST_LIT:
	case AST_ID:
	{
		err = analyse_expression(node, symtable, cntx);
		if (err != 0)
			return err;

		stmt_type = node->data_type;
		break;
	}

	default:
		return 6;
	}

	if (node->next != NULL)
		return analyse_node(node->next, symtable, cntx, locals, out_block_type);

	if (out_block_type != NULL)
		*out_block_type = stmt_type;

	return 0;
}

int semantic_analyze(AstNode* node, AVLNode** symtable)
{
	if (node == NULL || symtable == NULL)
		return 99;

	return analyse_node(node, symtable, NULL, NULL, NULL);
}
