/*
Projekt:Implementace překladače imperativního jazyka IFJ26
Popis : Vytvareni a uvolnovani uzlu AST
Soubor:  ast.h
Autor:  Nguyen Trung Hieu (xnguyet01)
*/
#include <stdlib.h>
#include "ast.h"
AstNode *ast_new_mode(AstKind kind, int line){
AstNode *node = calloc(1, sizeof(AstNode));
if (node == NULL){
    return NULL;
}
node->kind = kind;
node->line = line;
return node;
}   

void ast_free(AstNode *node){
if (node == NULL){
    return NULL;
}
ast_free(node->left);
ast_free(node->right);
ast_free(node->condition);
ast_free(node->body);
ast_free(node->else_body);
ast_free(node->parameters);
ast_free(node->arguments);
ast_free(node->value);
ast_free(node->next);

free(node->name);
free(node->str_val);
free(node);
}