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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * Stav lexera
 * ======================================================================== */

static FILE *inp; // vstup, z ktoreho citame znaky
static int line;  // cislo aktualneho riadku (pre chybove hlasky)

static Token ungot_tok; // token vrateny cez lexer_unget
static bool has_ungot;  // true = ungot_tok je platny a lexer_next ho vrati

#define MAX_INDENT 256

static int indent_stack[MAX_INDENT]; // urovne odsadenia, na spodku je vzdy 0
static int indent_top;               // index vrcholu zasobnika
static int pending_dedents;          // kolko DEDENT este treba poslat
static int paren_depth;              // hlbka zatvoriek ( [ - vnutri sa \n ignoruje
static bool at_line_start;           // sme na zaciatku noveho riadku?

/* ========================================================================
 * Tabulka klucovych slov
 * ======================================================================== */

/**
 * Jedna polozka tabulky klucovych slov: text slova a k nemu typ tokenu.
 */
typedef struct {
  const char *text;
  TokenType type;
} Keyword;

/**
 * Tabulka vsetkych klucovych slov jazyka IFJ26 (vratane rozsireni).
 * Pouziva ju find_keyword na rozlisenie klucoveho slova od identifikatora.
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
 * Vyhlada slovo v tabulke klucovych slov.
 *
 * @param word  hladane slovo
 * @return      typ klucoveho slova (napr. TOK_KW_WHILE),
 *              alebo TOK_ID, ak slovo v tabulke nie je
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

/* ========================================================================
 * Pomocne funkcie
 * ======================================================================== */

/**
 * Pozrie sa na dalsi znak na vstupe. Ak je to ocakavany znak, precita ho,
 * inak ho vrati spat do vstupu. Pouziva sa pri dvojznakovych operatoroch,
 * napr. ci za '<' nasleduje '=' (teda "<=").
 *
 * @param expected  ocakavany znak
 * @return          true = znak sedel a bol precitany, false = nesedel
 */
static bool match(int expected) {
  int next = fgetc(inp);
  if (next == expected) {
    return true;
  }
  ungetc(next, inp);
  return false;
}

/* ========================================================================
 * Komentare
 * ======================================================================== */

/**
 * Preskoci riadkovy komentar (uvodne "//" uz je precitane) az po koniec
 * riadku. Samotny koniec riadku necha na vstupe, aby z neho vznikol NEWLINE.
 */
static void skip_line_comment(void) {
  int c = fgetc(inp);
  while (c != '\n' && c != EOF) {
    c = fgetc(inp);
  }
  ungetc(c, inp);
}

/**
 * Preskoci blokovy komentar (uvodne "/" a "*" uz su precitane), vratane
 * vnorenych komentarov. Hlbku vnorenia pocita pocitadlo depth.
 *
 * @param had_newline  sem zapise, ci komentar obsahoval koniec riadku
 * @return             0 = OK, 1 = neukonceny komentar (lexikalna chyba)
 */
static int skip_block_comment(bool *had_newline) {
  int depth = 1; // uz sme vnutri jedneho komentara
  *had_newline = false;

  while (true) {
    int c = fgetc(inp);

    if (c == EOF) {
      return 1; // komentar nebol ukonceny
    }
    if (c == '\n') {
      line++;
      *had_newline = true;
    } else if (c == '/' && match('*')) {
      depth++; // vnoreny komentar
    } else if (c == '*' && match('/')) {
      depth--;
      if (depth == 0) {
        return 0;
      }
    }
  }
}

/**
 * Po viacriadkovom blokovom komentari uz na riadku nesmie byt ziadny token,
 * iba medzery a dalsie komentare. Koniec riadku necha na vstupe.
 *
 * @return  0 = OK, 1 = za komentarom je token (lexikalna chyba)
 */
static int check_rest_of_line(void) {
  int c = fgetc(inp);
  while (c == ' ' || c == '\t') {
    c = fgetc(inp);
  }

  if (c == '\n' || c == EOF) {
    ungetc(c, inp);
    return 0;
  }

  if (c == '/') {
    if (match('/')) {
      skip_line_comment();
      return 0;
    }
    if (match('*')) {
      bool had_newline;
      int err = skip_block_comment(&had_newline);
      if (err != 0) {
        return err;
      }
      return check_rest_of_line();
    }
  }

  return 1; // za viacriadkovym komentarom nasleduje token
}

/* ========================================================================
 * Slova: identifikatory a klucove slova
 * ======================================================================== */

/**
 * Precita cele slovo, ktore zacina znakom c, a urci, ci je to
 * samotne "_", klucove slovo alebo identifikator.
 *
 * @param c    prvy znak slova (pismeno alebo '_'), uz precitany
 * @param out  sem sa zapise token
 * @return     0 = OK, 99 = chyba alokacie
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
      return 99; // chyba alokacie pamate
    }
    c = fgetc(inp);
  }

  // Cyklus skoncil na znaku, ktory do slova nepatri (napr. medzera).
  // Ten patri dalsiemu tokenu, preto ho vratime spat do vstupu.
  ungetc(c, inp);

  // Samotne "_" nie je identifikator, je to specialny token
  if (strcmp(word.data, "_") == 0) {
    out->type = TOK_UNDERSCORE;
    dynstr_free(&word);
    return 0;
  }

  // Ak je slovo v tabulke klucovych slov, vratime jeho typ.
  // Text uz netreba, typ tokenu staci, preto ho uvolnime.
  TokenType type = find_keyword(word.data);
  if (type != TOK_ID) {
    out->type = type;
    dynstr_free(&word);
    return 0;
  }

  // Inak je to identifikator. Jeho meno potrebuje parser,
  // preto text odovzdame v tokene a NEuvolnujeme ho.
  out->type = TOK_ID;
  out->value.str_val = word.data;
  return 0;
}

/**
 * Precita anotaciu @main (znak '@' uz je precitany). Hned za '@' musi byt
 * presne slovo "main", bez medzery a bez dalsich znakov identifikatora.
 *
 * @param out  sem sa zapise token TOK_AT_MAIN
 * @return     0 = OK, 1 = lexikalna chyba (napr. "@ main" alebo "@mainx")
 */
static int read_at_main(Token *out) {
  const char *expected = "main";
  for (int i = 0; expected[i] != '\0'; i++) {
    if (fgetc(inp) != expected[i]) {
      return 1;
    }
  }

  // za "main" uz nesmie pokracovat identifikator (napr. "@mainx")
  int c = fgetc(inp);
  if (isalnum(c) || c == '_') {
    return 1;
  }
  ungetc(c, inp);

  out->type = TOK_AT_MAIN;
  return 0;
}

/* ========================================================================
 * Cisla: konecny automat pre celociselne a desatinne literaly
 * ======================================================================== */

/**
 * Stavy automatu pre cisla.
 * Koncove stavy (v nich moze cislo skoncit): NUM_ZERO, NUM_INT, NUM_FRAC,
 * NUM_EXP. Z ostatnych stavov skoncit nemozno, to je lexikalna chyba.
 */
typedef enum {
  NUM_ZERO,      // 0 na zaciatku
  NUM_INT,       // citam cislice celeho cisla
  NUM_INT_US,    // posledny znak bol '_'
  NUM_DOT,       // precitana '.', musi nasledovat cislica
  NUM_FRAC,      // citam desatinnu cast
  NUM_FRAC_US,   // posledny znak v desatinnej casti bol '_'
  NUM_EXP_START, // precitane e/E, musi ist znamienko alebo cislica
  NUM_EXP_SIGN,  // precitane +/-, musi ist cislica
  NUM_EXP,       // cislice exponentu
  NUM_EXP_US,    // '_' v exponente
} NumState;

/**
 * Ukonci celociselny literal: prevedie text na cislo a vytvori token.
 * Ak hned za cislom nasleduje pismeno alebo cislica (napr. "12abc"),
 * je to lexikalna chyba.
 *
 * @param num  doteraz poskladane cislice (bez '_'); funkcia ho uvolni
 * @param c    prvy znak za cislom
 * @param out  sem sa zapise token
 * @return     0 = OK, 1 = lexikalna chyba
 */
static int finish_int(DynStr *num, int c, Token *out) {
  if (isalnum(c)) {
    dynstr_free(num);
    return 1;
  }
  ungetc(c, inp);
  out->type = TOK_LIT_INT;
  out->value.int_val = strtoll(num->data, NULL, 10);
  dynstr_free(num);
  return 0;
}

/**
 * Ukonci desatinny literal, rovnako ako finish_int, len vysledok je Double.
 *
 * @param num  doteraz poskladany text cisla (bez '_'); funkcia ho uvolni
 * @param c    prvy znak za cislom (za priponou d/D, ak bola)
 * @param out  sem sa zapise token
 * @return     0 = OK, 1 = lexikalna chyba
 */
static int finish_double(DynStr *num, int c, Token *out) {
  if (isalnum(c)) {
    dynstr_free(num);
    return 1;
  }
  ungetc(c, inp);
  out->type = TOK_LIT_DOUBLE;
  out->value.double_val = strtod(num->data, NULL);
  dynstr_free(num);
  return 0;
}

/**
 * Precita cislo pomocou konecneho automatu (stavy NumState).
 * Podporuje celociselne literaly (42, 1_000), desatinne (1.5, .5),
 * exponent (1e10, 2.5E-3) a priponu d/D (1.5d).
 *
 * @param c    prvy znak cisla (cislica alebo '.'), uz precitany
 * @param out  sem sa zapise token
 * @return     0 = OK, 1 = lexikalna chyba, 99 = chyba alokacie
 */
static int read_number(int c, Token *out) {
  DynStr num; // cislice bez podciarknikov, napr. "1_000" -> "1000"
  if (!dynstr_init(&num)) {
    return 99;
  }

  // Zaciatocny stav podla prveho znaku
  NumState state;
  if (c == '0') {
    state = NUM_ZERO;
  } else if (c == '.') {
    state = NUM_DOT;
  } else {
    state = NUM_INT;
  }

  // Prvy znak patri do cisla
  if (!dynstr_append(&num, c)) {
    dynstr_free(&num);
    return 99;
  }

  // Citame znak po znaku a podla aktualneho stavu rozhodneme o prechode
  while (true) {
    c = fgetc(inp);
    bool keep = false; // ma sa znak c pridat do cisla?

    switch (state) {
    case NUM_ZERO:
      // Za nulou na zaciatku nesmie ist dalsia cislica ani '_' (napr. "007")
      if (isdigit(c) || c == '_') {
        dynstr_free(&num);
        return 1;
      } else if (c == '.') {
        keep = true;
        state = NUM_DOT;
        break;
      } else if (c == 'e' || c == 'E') {
        keep = true;
        state = NUM_EXP_START;
        break;
      } else if (c == 'd' || c == 'D') {
        // pripona d znamena Double, samotnu priponu do cisla nepridavame
        return finish_double(&num, fgetc(inp), out);
      } else {
        return finish_int(&num, c, out);
      }

    case NUM_INT:
      if (isdigit(c)) {
        keep = true;
        break;
      } else if (c == '_') {
        // podciarknik do cisla nepridavame, len si zapamatame, ze bol
        state = NUM_INT_US;
        break;
      } else if (c == '.') {
        keep = true;
        state = NUM_DOT;
        break;
      } else if (c == 'e' || c == 'E') {
        keep = true;
        state = NUM_EXP_START;
        break;
      } else if (c == 'd' || c == 'D') {
        return finish_double(&num, fgetc(inp), out);
      } else {
        return finish_int(&num, c, out);
      }

    case NUM_INT_US:
      // Za '_' musi ist dalsi '_' alebo cislica (napr. "1_" je chyba)
      if (c == '_') {
        break;
      } else if (isdigit(c)) {
        keep = true;
        state = NUM_INT;
        break;
      }
      dynstr_free(&num);
      return 1;

    case NUM_DOT:
      // Za bodkou musi ist cislica (napr. "1." je chyba)
      if (isdigit(c)) {
        keep = true;
        state = NUM_FRAC;
        break;
      } else {
        dynstr_free(&num);
        return 1;
      }

    case NUM_FRAC:
      if (isdigit(c)) {
        keep = true;
        break;
      } else if (c == '_') {
        state = NUM_FRAC_US;
        break;
      } else if (c == 'e' || c == 'E') {
        keep = true;
        state = NUM_EXP_START;
        break;
      } else if (c == 'd' || c == 'D') {
        return finish_double(&num, fgetc(inp), out);
      } else {
        return finish_double(&num, c, out);
      }

    case NUM_FRAC_US:
      if (c == '_') {
        break;
      } else if (isdigit(c)) {
        keep = true;
        state = NUM_FRAC;
        break;
      } else {
        dynstr_free(&num);
        return 1;
      }

    case NUM_EXP_START:
      // Za e/E musi ist znamienko alebo cislica
      if (c == '+' || c == '-') {
        keep = true;
        state = NUM_EXP_SIGN;
        break;
      } else if (isdigit(c)) {
        keep = true;
        state = NUM_EXP;
        break;
      } else {
        dynstr_free(&num);
        return 1; // 1e bez cisla je chyba
      }

    case NUM_EXP_SIGN:
      // Za znamienkom exponentu musi ist cislica
      if (isdigit(c)) {
        keep = true;
        state = NUM_EXP;
        break;
      } else {
        dynstr_free(&num);
        return 1;
      }

    case NUM_EXP:
      if (isdigit(c)) {
        keep = true;
        break;
      } else if (c == '_') {
        state = NUM_EXP_US;
        break;
      } else if (c == 'd' || c == 'D') {
        return finish_double(&num, fgetc(inp), out);
      } else {
        return finish_double(&num, c, out);
      }

    case NUM_EXP_US:
      if (c == '_') {
        break;
      } else if (isdigit(c)) {
        keep = true;
        state = NUM_EXP;
        break;
      } else {
        dynstr_free(&num);
        return 1;
      }
    }

    // Znak pridame az tu, aby bola kontrola chyby pamate na jednom mieste
    if (keep && !dynstr_append(&num, c)) {
      dynstr_free(&num);
      return 99;
    }
  }
}

/* ========================================================================
 * Retazce
 * ======================================================================== */

/**
 * Precita viacriadkovy retazec """...""" (uvodne """ su uz precitane).
 * Obsah sa berie doslova: escape sekvencie sa nevyhodnocuju
 * a retazec moze obsahovat konce riadkov.
 *
 * @param out  sem sa zapise token
 * @return     0 = OK, 1 = neukonceny retazec, 99 = chyba alokacie
 */
static int read_multiline_string(Token *out) {
  DynStr str;
  if (!dynstr_init(&str)) {
    return 99;
  }

  int quotes = 0; // kolko '"' sme videli za sebou

  while (true) {
    int c = fgetc(inp);

    // Uvodzovky zatial len pocitame, az tri za sebou ukoncia retazec
    if (c == '"') {
      quotes++;
      if (quotes == 3) {
        out->type = TOK_LIT_STRING;
        out->value.str_val = str.data;
        return 0;
      }
      continue;
    }

    // Prisiel iny znak, takze uvodzovky pred nim patria do obsahu
    for (int i = 0; i < quotes; i++) {
      if (!dynstr_append(&str, '"')) {
        dynstr_free(&str);
        return 99;
      }
    }
    quotes = 0;

    // Koniec suboru vnutri retazca = neukonceny retazec
    if (c == EOF) {
      dynstr_free(&str);
      return 1;
    }

    // Retazec zabera viac riadkov, aby dalsie tokeny mali spravny riadok
    if (c == '\n') {
      line++;
    }

    if (!dynstr_append(&str, c)) {
      dynstr_free(&str);
      return 99;
    }
  }
}

/**
 * Precita retazcovy literal (uvodna '"' je uz precitana).
 * Najprv rozlisi prazdny retazec "", viacriadkovy """...""" a obycajny "...".
 * V obycajnom retazci prevadza escape sekvencie \n \t \r \" \\ na znaky.
 *
 * @param out  sem sa zapise token
 * @return     0 = OK, 1 = lexikalna chyba, 99 = chyba alokacie
 */
static int read_string(Token *out) {
  // Pozrieme sa na dalsie dva znaky: "" alebo """ alebo obycajny retazec
  int c2 = fgetc(inp);
  if (c2 == '"') {
    int c3 = fgetc(inp);
    if (c3 == '"') {
      return read_multiline_string(out); // """
    }

    // Prazdny retazec "": treti znak uz k nemu nepatri
    ungetc(c3, inp);
    DynStr empty;
    if (!dynstr_init(&empty)) {
      return 99;
    }
    out->type = TOK_LIT_STRING;
    out->value.str_val = empty.data;
    return 0;
  } else {
    ungetc(c2, inp); // obycajny retazec, pokracuj normalne
  }

  DynStr str;
  if (!dynstr_init(&str)) {
    return 99;
  }

  while (true) {
    int c = fgetc(inp);

    if (c == '"') { // koniec retazca
      out->type = TOK_LIT_STRING;
      out->value.str_val = str.data;
      return 0;
    }

    // Obycajny retazec nesmie obsahovat koniec riadku ani skoncit so suborom
    if (c == '\n' || c == EOF) {
      dynstr_free(&str);
      return 1; // neukonceny retazec -> chyba
    }

    // Escape sekvencia: dvojicu znakov "\x" nahradime jednym znakom
    if (c == '\\') {
      int next = fgetc(inp);
      int translated; // prelozeny znak, ktory pridame do retazca
      switch (next) {
      case 'n':
        translated = '\n';
        break;
      case 't':
        translated = '\t';
        break;
      case 'r':
        translated = '\r';
        break;
      case '"':
        translated = '"';
        break;
      case '\\':
        translated = '\\';
        break;
      default:
        dynstr_free(&str);
        return 1; // neznama escape sekvencia
      }
      if (!dynstr_append(&str, translated)) {
        dynstr_free(&str);
        return 99;
      }
      continue;
    }

    if (!dynstr_append(&str, c)) {
      dynstr_free(&str);
      return 99;
    }
  }
}

/* ========================================================================
 * Operatory a oddelovace
 * ======================================================================== */

/**
 * Rozpozna operator alebo oddelovac, ktory zacina znakom c.
 * Pri dvojznakovych operatoroch (napr. "<=") sa pozrie na dalsi znak
 * pomocou match.
 *
 * @param c    prvy znak operatora, uz precitany
 * @param out  sem sa zapise token
 * @return     0 = OK, 1 = neznamy znak (lexikalna chyba)
 */
static int read_operator(int c, Token *out) {
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

  // jedno- alebo dvojznakove
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

  // Znak nepatri ziadnemu tokenu
  return 1;
}

/* ========================================================================
 * Odsadenie: INDENT a DEDENT
 * ======================================================================== */

/**
 * Spracuje odsadenie na zaciatku riadku a porovna ho so zasobnikom
 * urovni odsadenia. Prazdne riadky preskoci (odsadenie nemenia).
 *
 * @param out  sem zapise INDENT alebo DEDENT, ak vznikne
 * @return     0 = poslal INDENT/DEDENT, -1 = ziadny token (rovnake
 *             odsadenie alebo EOF), 1 = lexikalna chyba (tabulator,
 *             vynorenie na neexistujucu uroven), 99 = prilis hlboke vnorenie
 */
static int read_indent(Token *out) {
  while (true) {
    // Spocitame medzery na zaciatku riadku
    int spaces = 0;
    int c = fgetc(inp);
    while (c == ' ') {
      spaces++;
      c = fgetc(inp);
    }

    if (c == '\t') {
      return 1; // tabulator v odsadeni je lexikalna chyba
    }

    // Komentare na zaciatku riadku. Riadok, na ktorom je iba komentar,
    // sa sprava ako prazdny a odsadenie nemeni.
    while (c == '/') {
      int next = fgetc(inp);
      if (next == '/') {
        skip_line_comment();
        c = fgetc(inp); // '\n' alebo EOF
        break;
      }
      if (next != '*') {
        // logicky riadok nemoze zacinat operatorom (napr. '/')
        ungetc(next, inp);
        return 2;
      }
      bool had_newline;
      int err = skip_block_comment(&had_newline);
      if (err != 0) {
        return err;
      }
      if (had_newline) {
        // viacriadkovy komentar = koniec riadku, za nim nesmie byt token
        err = check_rest_of_line();
        if (err != 0) {
          return err;
        }
        c = fgetc(inp); // '\n' alebo EOF
        break;
      }
      // komentar na jednom riadku je ako medzera, pokracujeme za nim
      c = fgetc(inp);
      while (c == ' ' || c == '\t') {
        c = fgetc(inp);
      }
    }

    if (c == '\n') {
      line++; // prazdny riadok odsadenie nemeni, ideme na dalsi
      continue;
    }
    if (c == EOF) {
      return -1; // DEDENTy na konci suboru riesi lexer_next
    }

    ungetc(c, inp); // prvy znak tokenu vratime spat
    at_line_start = false;
    out->line = line;
    int top = indent_stack[indent_top];

    // Vacsie odsadenie -> zacina novy blok
    if (spaces > top) {
      if (indent_top + 1 >= MAX_INDENT) {
        return 99;
      }
      indent_top++;
      indent_stack[indent_top] = spaces;
      out->type = TOK_INDENT;
      return 0;
    }

    // Mensie odsadenie -> konci jeden alebo viac blokov
    if (spaces < top) {
      int count = 0;
      while (indent_stack[indent_top] > spaces) {
        indent_top--;
        count++;
      }
      // Vynorili sme sa na uroven, ktora predtym nebola (napr. 2, ked
      // zasobnik je [0, 4]) -> chyba
      if (indent_stack[indent_top] != spaces) {
        return 1;
      }
      out->type = TOK_DEDENT;      // jeden DEDENT posleme hned
      pending_dedents = count - 1; // ostatne pri dalsich volaniach
      return 0;
    }

    return -1; // rovnake odsadenie, ziadny token
  }
}

/* ========================================================================
 * Verejne funkcie (rozhranie je popisane v lexer.h)
 * ======================================================================== */

int lexer_init(FILE *input) {
  inp = input;
  line = 1;
  has_ungot = false;
  indent_stack[0] = 0;
  indent_top = 0;
  pending_dedents = 0;
  paren_depth = 0;
  at_line_start = true;
  return 0;
}

/*
 * Postup: preskoci medzery a podla prveho znaku rozhodne, aky token cita.
 * Samotne citanie robia pomocne funkcie read_word, read_number, read_string
 * a read_operator. Ak znak nepozna ani read_operator, je to lexikalna chyba.
 */
int lexer_next(Token *out) {
  // Ak parser vratil token cez lexer_unget, dame mu najprv ten
  if (has_ungot) {
    *out = ungot_tok;
    has_ungot = false;
    return 0;
  }

  out->newline_before = false;

  // Este nevyslane DEDENTy z minuleho vynorenia o viac urovni
  if (pending_dedents > 0) {
    pending_dedents--;
    out->type = TOK_DEDENT;
    out->line = line;
    return 0;
  }

  // Na zaciatku riadku (mimo zatvoriek) najprv spracujeme odsadenie
  if (at_line_start && paren_depth == 0) {
    int r = read_indent(out);
    if (r != -1) {
      return r;
    }
  }

  int c = fgetc(inp);

  // Preskocime medzery (v strede riadku je biely znak aj tabulator)
  while (c == ' ' || c == '\t') {
    c = fgetc(inp);
  }

  // Komentar v strede riadku
  if (c == '/') {
    int next = fgetc(inp);
    if (next == '/') {
      // riadkovy komentar: zvysok riadku zahodime, '\n' ostane na vstupe
      skip_line_comment();
      c = fgetc(inp);
    } else if (next == '*') {
      bool had_newline;
      int err = skip_block_comment(&had_newline);
      if (err != 0) {
        return err;
      }
      if (had_newline) {
        // viacriadkovy komentar = koniec riadku, za nim nesmie byt token
        err = check_rest_of_line();
        if (err != 0) {
          return err;
        }
      }
      // komentar je ako biely znak, citame dalsi token
      return lexer_next(out);
    } else {
      ungetc(next, inp); // nie je to komentar, ale delenie '/'
    }
  }
  out->line = line;

  // Koniec suboru: najprv uzavrieme vsetky otvorene bloky (DEDENT),
  // az potom posleme EOF
  if (c == EOF) {
    if (indent_top > 0) {
      indent_top--;
      out->type = TOK_DEDENT;
      return 0;
    }
    out->type = TOK_EOF;
    return 0;
  }

  // Koniec riadku
  if (c == '\n') {
    line++;
    // Vnutri zatvoriek je koniec riadku iba biely znak
    if (paren_depth > 0) {
      int r = lexer_next(out);
      out->newline_before = true; // kvoli koncovej ciarke v parseri
      return r;
    }
    at_line_start = true;
    out->type = TOK_NEWLINE;
    return 0;
  }

  // Slovo moze zacinat pismenom alebo podciarknikom.
  // Ak tak zacina, je to identifikator alebo klucove slovo.
  if (isalpha(c) || c == '_') {
    return read_word(c, out);
  }

  // Cislo zacina cislicou
  if (isdigit(c)) {
    return read_number(c, out);
  }

  // ".5" je cislo, ale samotna '.' je operator: pozrieme sa na dalsi znak
  if (c == '.') {
    int next = fgetc(inp);
    ungetc(next, inp);
    if (isdigit(next)) {
      return read_number(c, out);
    }
  }

  // Retazec zacina uvodzovkou
  if (c == '"') {
    return read_string(out);
  }

  // Anotacia @main
  if (c == '@') {
    return read_at_main(out);
  }

  // Vsetko ostatne je operator, oddelovac alebo neznamy znak
  int r = read_operator(c, out);

  // Pocitame hlbku zatvoriek, aby sme vnutri ignorovali konce riadkov
  if (r == 0) {
    if (out->type == TOK_LPAREN || out->type == TOK_LBRACKET) {
      paren_depth++;
    } else if ((out->type == TOK_RPAREN || out->type == TOK_RBRACKET) &&
               paren_depth > 0) {
      paren_depth--;
    }
  }
  return r;
}

/*
 * Token si iba odlozime. Dalsie volanie lexer_next ho vrati namiesto
 * citania zo vstupu. Naraz sa da vratit len jeden token.
 */
void lexer_unget(Token *tok) {
  ungot_tok = *tok;
  has_ungot = true;
}

/*
 * Lexer si nedrzi ziadnu dynamicky alokovanu pamat (zasobnik odsadeni je
 * staticke pole a text v tokenoch patri parseru), preto staci vynulovat stav.
 */
void lexer_free(void) {
  has_ungot = false;
  indent_top = 0;
  pending_dedents = 0;
  paren_depth = 0;
}
