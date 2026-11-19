/*
Projekt:Implementace překladače imperativního jazyka IFJ26
Rozhrani abstraktniho syntaktickeho stromu
Soubor:  ast.c
Autor:   xnguyet01 Nguyen Trung Hieu 
*/
#ifndef AST_H
#define AST_H
#include <stdbool.h>

// Druhy uzlu stromu
typedef enum {
    AST_PROGRAM,
    AST_FUNC_DEF,
    AST_PARAMETR,
    AST_VAR_DEF,
    AST_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_RETURN,
    AST_FUNC_CALL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_LIT,
    AST_ID,
}AstKind;

// Datove typy jazyka IFJ26
typedef enum {
TYPE_NONE, // typ neni napsany / jeste neznamy
TYPE_INT,
TYPE_STRING,
TYPE_DOUBLE,
TYPE_BOOL,
TYPE_UNIT,
}Datatype;

// Operatory ve vyrazech
typedef enum{
    OP_NONE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG, // unarni minus
    OP_EQUAL,
    OP_NOTEQUAL,
    OP_LESSTHAN,
    OP_GREATERTHAN,
    OP_LESSOREQUAL,
    OP_GREATEROREQUAL,

}Optype;

// Uzel stromu. Kazdy druh uzlu pouziva jen nektera pole, ostatni jsou NULL / 0
typedef struct AstNode{
    AstKind kind;
    int line;
    struct AstNode *next; // dalsi uzel v seznamu
    char *name;
    Datatype data_type; ; // u definic napsany typ u vyrazu typ vysledku
    double double_val;
    char *str_val;

    Optype op;
    struct AstNode *left; //u UNARY_OP jediny operand
    struct AstNode *right;
    struct AstNode *condition;
    struct AstNode *body;
    struct AstNode *else_body;
    struct AstNode *parameters;
    struct AstNode *arguments;
    struct AstNode *value;

    bool is_mutable;
    bool is_main;

}AstNode;

// Vytvori novy uzel, Vraci NULL, kdyz se nepovede alokace
AstNode *ast_new_node(AstKind kind, int line);

// Uvolni uzel, vsechny jeho potomky i dalsi uzly pres next
void ast_free(AstNode *node);

#endif

