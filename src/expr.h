/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Soubor:  expr.h
 * Popis:   Rozhrani precedencni analyzy vyrazu
 * Autor:   Nguyen Trung Hieu (xnguyet01)
 */

#ifndef EXPR_H
#define EXPR_H

#include "ast.h"

// Precte vyraz precedencni analyzou a vrati jeho AST.
// Skonci na prvnim tokenu, ktery do vyrazu nepatri (then, do, NEWLINE, ...).
AstNode* parse_expression(void);

#endif
