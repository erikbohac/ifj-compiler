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

/**
 * pomocna struktura Keyword = {text, type}
 * pre porovnavanie
 */
typedef struct {
  const char *text;
  TokenType type;
} Keyword;

/**
 * keywords[] = pole typu Keyword
 */
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
/**
 * Tuto funkciu vyuzivame v int lexer_next(Token *out)
 * Ked zistujeme dvojite operatory "<=" "<-" ...
 * next = dalsi znak, expected = napr "=" za "<"
 */
static bool match(int expected) {
  int next = fgetc(inp);
  if (next == expected) {
    return true;
  }
  ungetc(next, inp);
  return false;
}

/**
 * Funkcia ktora hlada v poli keywords[] slovo
 * Vrati typ TokenType z lexer.h
 */
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
/*
 * Postup:
 * 1. preskoci medzery
 * 2. podla prveho znaku rozhodne, aky token cita:
 *    a) EOF, koniec riadku
 *    b) slovo (identifikator alebo keyword)
 *    c) operator alebo oddelovac
 * 3. ak znak nepozna v
 */
int lexer_next(Token *out) {
  int c = fgetc(inp);
  // --- 1. preskocenie medzier ---
  while (c == ' ') {
    c = fgetc(inp);
  }
  out->line = line;
  // --- 2a. koniec suboru ---
  if (c == EOF) {
    out->type = TOK_EOF;
    return 0; // OK
  }
  // --- 2a. novy riadok ---
  if (c == '\n') {
    out->type = TOK_NEWLINE;
    line++;
    return 0; // OK
  }
  // Slovo moze zacinat pismenom alebo podciarknikom
  // Ak tak zacina, je to identifikator alebo klucove slovo
  if (isalpha(c) || c == '_') {
    // Dlzku slova dopredu nepozname, preto ho skaldame do dynamickeho retazca
    DynStr word;
    if (!dynstr_init(&word)) {
      return 99; // chyba inicializacie
    }

    // Pridavame znaky, kym patria do slova (pismena, cislice, podciarknik)
    while (isalnum(c) || c == '_') {
      if (!dynstr_append(&word, c)) {
        dynstr_free(&word);
        return 99; // chyba alokacie pameti
      }
      c = fgetc(inp);
    }

    // Cyklus skoncil na znaku ktory do slova nepatri napr medzera
    // Takze ho vratime spat do vstupu
    ungetc(c, inp);
    // Samotne "_" nieje slovo, je to specialny token
    if (strcmp(word.data, "_") == 0) {
      out->type = TOK_UNDERSCORE;
      dynstr_free(&word);
      return 0;
    }

    // ak je slovo v tabulke klucovych slov, vratime jeho typ
    // text uz netreba, typ tokenu staci, preto ho uvolnime
    TokenType type = find_keyword(word.data);
    if (type != TOK_ID) {
      out->type = type;
      dynstr_free(&word);
      return 0;
    }
    // Inak je to identifikator. Jehoo meno parser potrebuje
    // preto text odovzdame v tokene a neuvolnujeme ho
    out->type = TOK_ID;
    out->value.str_val = word.data;
    return 0;
  }
  switch (c) {
  // jednoznakove
  case '-':
    out->type = TOK_MINUS;
    return 0;
  case '/':
    out->type = TOK_DIV;
    return 0;
  case '.':
    out->type = TOK_DOT;
    return 0;
  case '*':
    out->type = TOK_MUL;
    return 0;
  case '(':
    out->type = TOK_LPAREN;
    return 0;
  case ')':
    out->type = TOK_RPAREN;
    return 0;
  case ':':
    out->type = TOK_COLON;
    return 0;
  case ',':
    out->type = TOK_COMMA;
    return 0;
  case ';':
    out->type = TOK_SEMICOLON;
    return 0;
  case '[':
    out->type = TOK_LBRACKET;
    return 0;
  case ']':
    out->type = TOK_RBRACKET;
    return 0;
  // dvojite operatory
  case '+':
    if (match('+')) {
      out->type = TOK_CONCAT; // ++
    } else {
      out->type = TOK_PLUS; // +
    }
    return 0;
  case '<':
    if (match('=')) {
      out->type = TOK_LE; // <=
    } else if (match('-')) {
      out->type = TOK_LARROW; // <-
    } else {
      out->type = TOK_LT; // <
    }
    return 0;
  case '>':
    if (match('=')) {
      out->type = TOK_GE; // >=
    } else {
      out->type = TOK_GT; // >
    }
    return 0;
  case '!':
    if (match('=')) {
      out->type = TOK_NEQ; // !=
    } else {
      out->type = TOK_NOT; // !
    }
    return 0;
  case '|':
    if (match('|')) {
      out->type = TOK_OR; // ||
    } else {
      out->type = TOK_PIPE; // |
    }
    return 0;
  case '=':
    if (match('=')) {
      out->type = TOK_EQ; // ==
    } else if (match('>')) {
      out->type = TOK_ARROW; // =>
    } else {
      out->type = TOK_ASSIGN; // =
    }
    return 0;
  case '&':
    if (match('&')) {
      out->type = TOK_AND; // &&
    } else {
      return 1; // samotne & nemoze byt, preto je to chyba
    }
    return 0;
  }
  return 1; // lexikalna chyba
}
void lexer_unget(Token *tok) { return; }

void lexer_free(void) { return; }