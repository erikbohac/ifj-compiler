/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:  parser.h
 * Autor:   Michal (xnguyet01)
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

extern Token current;

// Precte cely program a vrati koren AST.
AstNode *parse_program(void);

// Nacte dalsi token, pri chybe lexeru ukonci program.
void next_token(void);

// Vypise chybu a ukonci program s kodem 2.
void syntax_error(const char *msg);

// Vytvori uzel AST, pri chybe pameti ukonci program s kodem 99.
AstNode *new_node(AstKind kind);

#endif
