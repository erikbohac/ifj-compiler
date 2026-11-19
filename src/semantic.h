/**
 * Project: IFJ26
 * @file semantic.h
 *
 * @author xsmekav00, Veronika Smékalová
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"

#include <stdbool.h>

bool semantic_analyze(AstNode* root, AVLNode** symtable);

#endif