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

/*
 * Precita cele slovo, ktore zacina znakom c,
 * a urci, ci je to "_", klucove slovo alebo identifikator
 */
static int read_word(int c, Token *out) {
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
  // pre ID treba aj slovo ktore potom pojde do parseru
  out->type = TOK_ID;
  out->value.str_val = word.data;
  return 0;
}

typedef enum {
  NUM_ZERO,   // 0 na zaciatku
  NUM_INT,    // citam cislice celeho cisla
  NUM_INT_US, // posledny znak bol '_'
} NumState;

// Precita cislo, ktore zacina cislicou c (konecny automat).
static int read_number(int c, Token *out) {
  DynStr num; // cislice bez podciarknikov, napr. "1_000" -> "1000"
  if (!dynstr_init(&num)) {
    return 99;
  }

  NumState state = (c == '0') ? NUM_ZERO : NUM_INT;
  // pridaj prvu cislicu c do num
  if (!dynstr_append(&num, c)) {
    dynstr_free(&num);
    return 99;
  }

  while (true) {
    c = fgetc(inp);
    switch (state) {
    case NUM_ZERO:
      if (isdigit(c) || c == '_') {
        dynstr_free(&num);
        return 1;
      }
      ungetc(c, inp);
      // prevod z string -> long long
      out->value.int_val = strtoll(num.data, NULL, 10);
      out->type = TOK_LIT_INT;
      dynstr_free(&num);
      return 0;
    case NUM_INT:
      if (isdigit(c)) {
        dynstr_append(&num, c);
        break;
      } else if (c == '_') {
        state = NUM_INT_US;
        break;
      } else {
        ungetc(c, inp);
        out->value.int_val = strtoll(num.data, NULL, 10);
        out->type = TOK_LIT_INT;
        dynstr_free(&num);
        return 0;
      }

    case NUM_INT_US:
      if (c == '_') {
        break;
      } else if (isdigit(c)) {
        dynstr_append(&num, c);
        state = NUM_INT;
        break;
      }
      dynstr_free(&num);
      return 1;
    }
  }
}
static int read_operator(int c, Token *out) {
  // operatory
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
  return 1;
}
int lexer_next(Token *out) {
  int c = fgetc(inp);
  // preskocime medzery
  while (c == ' ') {
    c = fgetc(inp);
  }
  out->line = line;
  if (c == EOF) {
    out->type = TOK_EOF;
    return 0; // OK
  }
  if (c == '\n') {
    out->type = TOK_NEWLINE;
    line++;
    return 0; // OK
  }
  // Slovo moze zacinat pismenom alebo podciarknikom
  // Ak tak zacina, je to identifikator alebo klucove slovo
  if (isalpha(c) || c == '_') {
    return read_word(c, out);
  }
  if (isdigit(c)) {
    return read_number(c, out);
  }
  return read_operator(c, out);
}
void lexer_unget(Token *tok) { return; }

void lexer_free(void) { return; }