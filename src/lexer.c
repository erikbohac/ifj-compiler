/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Subor:   lexer.c
 * Popis:   Implementacia lexikalneho analyzatoru
 *
 * Autor:   Filip Slovak (xslovaf00)
 */

#include "lexer.h"
#include "dynstr.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static FILE *inp; // odkial citame
static int line;  // aktualny riadok

typedef struct {
  const char *text;
  TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"def", TOK_KW_DEF},         {"do", TOK_KW_DO},
    {"else", TOK_KW_ELSE},       {"if", TOK_KW_IF},
    {"import", TOK_KW_IMPORT},   {"return", TOK_KW_RETURN},
    {"then", TOK_KW_THEN},       {"val", TOK_KW_VAL},
    {"var", TOK_KW_VAR},         {"while", TOK_KW_WHILE},
    {"Int", TOK_KW_INT},         {"Double", TOK_KW_DOUBLE},
    {"String", TOK_KW_STRING},   {"Unit", TOK_KW_UNIT},
    {"Boolean", TOK_KW_BOOLEAN}, {"case", TOK_KW_CASE},
    {"class", TOK_KW_CLASS},     {"extension", TOK_KW_EXTENSION},
    {"for", TOK_KW_FOR},         {"List", TOK_KW_LIST},
    {"match", TOK_KW_MATCH},     {"type", TOK_KW_TYPE},
    {"true", TOK_KW_TRUE},       {"false", TOK_KW_FALSE},
};

// funkcia hlada v tabulke slovo
static TokenType find_keyword(const char *word) {
  int length = sizeof(keywords) / sizeof(keywords[0]);
  for (int i = 0; i < length; i++) {
    if (strcmp(word, keywords[i].text) == 0) {
      return keywords[i].type;
    }
  }
  return TOK_ID;
}

int lexer_init(FILE *input) {
  inp = input;
  line = 1;
  return 0;
}

int lexer_next(Token *out) {
  int c = fgetc(inp); // citanie znaku
  while (c == ' ') {  // preskocime medzery
    c = fgetc(inp);
  }
  out->line = line;
  if (c == EOF) { // ak je koniec vratime EOF
    out->type = TOK_EOF;
    return 0; // OK
  }
  if (c == '\n') { // newline
    out->type = TOK_NEWLINE;
    line++;
    return 0; // OK
  }
  if (isalpha(c) || c == '_') { // ak zacina slovo
    DynStr word;
    if (!dynstr_init(&word)) { // nepodarilo sa inicializovat
      return 99;
    }
    while (isalnum(c) || c == '_') {  // pokracujeme v citani slova
      if (!dynstr_append(&word, c)) { // nepodarilo sa appendnut
        dynstr_free(&word);
        return 99;
      }
      c = fgetc(inp);
    }
    ungetc(c, inp);                    // vrati znak ktory uz do slova nepatri
    if (strcmp(word.data, "_") == 0) { // slovo = samotne "_"
      out->type = TOK_UNDERSCORE;
      dynstr_free(&word);
      return 0;
    }
    TokenType type = find_keyword(word.data); // zistime ci je slovo keyword
    if (type != TOK_ID) {                     // je keyword
      out->type = type;
      dynstr_free(&word);
      return 0;
    }
    out->type = TOK_ID; // je normalne slovo
    out->value.str_val = word.data;
    return 0;
  }
  return 1; // lexikalna chyba
}
void lexer_unget(Token *tok) { return; }

void lexer_free(void) { return; }