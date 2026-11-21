/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Subor:   lexer.h
 * Popis:   Rozhranie lexikalneho analyzatoru
 *
 * Autor:   Filip Slovak (xslovaf00)
*/

#ifndef LEXER_H    // Include guard
#define LEXER_H
#include <stdio.h> // pre typ FILE

// Typy Tokenov
typedef enum{
    // Klucove slova
    TOK_KW_DEF,
    TOK_KW_DO,
    TOK_KW_ELSE,
    TOK_KW_IF,
    TOK_KW_IMPORT,
    TOK_KW_RETURN,
    TOK_KW_THEN,
    TOK_KW_VAL,
    TOK_KW_VAR,
    TOK_KW_WHILE,
    TOK_KW_INT,
    TOK_KW_DOUBLE,
    TOK_KW_STRING,
    TOK_KW_UNIT,
    // Klucove slova rozsirenie
    TOK_KW_BOOLEAN,
    TOK_KW_CASE,
    TOK_KW_CLASS,
    TOK_KW_EXTENSION,
    TOK_KW_FOR,
    TOK_KW_LIST,
    TOK_KW_MATCH,
    TOK_KW_TYPE,
    TOK_KW_TRUE,
    TOK_KW_FALSE,
    // Identifikator
    TOK_ID,     // meno funkcie, premenna
    // Literaly
    TOK_LIT_INT,    // 42, 1_000
    TOK_LIT_DOUBLE, // 1.0, .5, 1e10, 1.1E2
    TOK_LIT_STRING, // "Ahoj", "Riadok 1 \n Riadok 2", "Povedal \"ahoj\"", """viac riadkovy"""
    // Operatory
    TOK_PLUS,   // "+"
    TOK_MINUS,  // "-"
    TOK_MUL,    // "*"
    TOK_DIV,    // "/"
    TOK_LT,     // "<"
    TOK_GT,     // ">"
    TOK_LE,     // "<="
    TOK_GE,     // ">="
    TOK_EQ,     // "=="
    TOK_NEQ,    // "!="
    TOK_ASSIGN, // "="
    // Oddelovace
    TOK_LPAREN,    // "("
    TOK_RPAREN,    // ")"
    TOK_COLON,     // ":"
    TOK_COMMA,     // ","
    TOK_SEMICOLON, // ";"
    TOK_DOT,       // "."
    // Specialne
    TOK_AT_MAIN,   // "@main"
    // Rozlozenie
    TOK_NEWLINE,   // "\n" alebo koniec logickeho riadku
    TOK_INDENT,    // "{" zaciatok bloku
    TOK_DEDENT,    // "}" koniec blokum
    TOK_EOF,       // "End Of File"
    // Rozsirenie
    TOK_NOT,        // "!"
    TOK_AND,        // "&&"
    TOK_OR,         // "||"
    TOK_ARROW,      // "=>"
    TOK_LARROW,     // "<-"
    TOK_PIPE,       // "|"
    TOK_LBRACKET,   // "["
    TOK_RBRACKET,   // "]"
    TOK_CONCAT,     // "++"
    TOK_UNDERSCORE, // "_"
} TokenType;

// Token
typedef struct{
    TokenType type;           // tabulka hore
    union {
        long long int_val;    // pre INT literal
        double    double_val; // pre DOUBLE literal
        char      *str_val;   // pre ID a STRING literal
    } value;                  // cislo alebo text
    int line;                 // cislo riadku, pre chybove hlasky
    int newline_before;       // bol pred tokenom koniec riadku?
} Token;

// funkcie ktore bude Parser Volat

/**
 * Pripravi lexer na citanie
 * 
 * @param input  Subor FILE z ktoreho cita (stdin)
 * @return       0 = OK, 99 = chyba alokacie (malloc)
 */
int lexer_init(FILE *input);

/**
 * Nacita dalsi token zo vstupu.
 * 
 * @param out  sem sa zapise nacitany token
 * @return     0 = OK, 1 = lexikalna chyba, 99 = chyba alokacie (malloc)
 */
int lexer_next(Token *out);

/**
 * Vrati jeden token spat.
 * 
 * @param tok  token, ktory sa vracia
 * @note       naraz sa da vratit len jeden token.
 */
void lexer_unget(Token *tok);

void lexer_free(void); // uvolni vsetku pamat, ktoru lexer pouziva (vola sa na konci prekladu)

#endif