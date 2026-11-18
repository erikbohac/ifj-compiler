/*Projekt:Implementace překladače imperativního jazyka IFJ26
Rozhrani abstraktniho syntaktickeho stromu
Soubor:  ast.c
Autor:   xnguyet01 Nguyen Trung Hieu 
*/
#include <stdlib.h> // calloc, free
#include "ast.h"

// Vytvori novy uzel, vsechna pole nastavi na 0 / NULL.
// Vraci NULL, kdyz se nepovede alokace.
AstNode *ast_new_node(AstKind kind, int line)
{
    AstNode *node = calloc(1, sizeof(AstNode));
    if (node == NULL)
    {
        return NULL;
    }

    node->kind = kind;
    node->line = line;
    return node;
}

// Uvolni uzel, vsechny jeho potomky i dalsi uzly pres next.
void ast_free(AstNode *node)
{
    if (node == NULL)
    {
        return;
    }

    // nejdriv potomci
    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->condition);
    ast_free(node->body);
    ast_free(node->else_body);
    ast_free(node->parameters);
    ast_free(node->arguments);
    ast_free(node->value);
    ast_free(node->next);

    // pak texty
    free(node->name);
    free(node->str_val);

    // nakonec samotny uzel
    free(node);
}