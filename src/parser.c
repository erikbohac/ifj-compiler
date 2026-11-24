/*
* Projekt: Implementace prekladace imperativniho jazyka IFJ26
* Soubor:  parser.c
* Autor:   Nguyen Trung Hieu (xnguyet01)
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

Token current;

void next_token(void)
{
    int result = lexer_next(&current);
    if (result != 0)
    {
        exit(result);
    }
}

void syntax_error(const char *msg)
{
    fprintf(stderr, "Syntakticka chyba na radku %d: %s\n", current.line, msg);
    exit(2);
}

AstNode *new_node(AstKind kind)
{
    AstNode *node = ast_new_node(kind, current.line);
    if (node == NULL)
    {
        exit(99);
    }
    return node;
}

// Zkontroluje typ aktualniho tokenu a posune se dal.
static void expect(TokenType type, const char *msg)
{
    if (current.type != type)
    {
        syntax_error(msg);
    }
    next_token();
}

// Preskoci prazdne radky.
static void skip_newlines(void)
{
    while (current.type == TOK_NEWLINE)
    {
        next_token();
    }
}

// <prolog> -> import ifj26.*
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

// program prolog funkce_list EOF
AstNode *parse_program(void)
{
    next_token();
    skip_newlines();

    AstNode *program = new_node(AST_PROGRAM);
    parse_prolog();

    // Za prologem musi nasledovat konec radku (nebo EOF)
    if (current.type != TOK_NEWLINE && current.type != TOK_EOF)
    {
        syntax_error("ocekavan novy radek za prologem");
    }
    skip_newlines();

    // TODO: definice funkci
    if (current.type != TOK_EOF)
    {
        syntax_error("ocekavan konec souboru");
    }

    return program;
}