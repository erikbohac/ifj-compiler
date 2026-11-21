/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Subor:   dynstr.h
 * Popis:   Rozhranie dynamickeho retazca
 *
 * Autor:   Filip Slovak (xslovaf00)
*/

#ifndef DYNSTR_H      // Include guard
#define DYNSTR_H
#include <stddef.h>  // Pre size_t
#include <stdbool.h> // Pre bool

typedef struct{
    char *data;
    size_t length;
    size_t capacity;
} DynStr;


/**
 * Inicializuje prazdny retazec a alokuje zaciatocnu pamat.
 * 
 * @param str  retazec ktory sa inicializuje
 * @return     true = OK, false = chyba alokacie
 */
bool dynstr_init(DynStr *str);

/**
 * Prida jeden znak na koniec retazca, ak je plno retazec sa zvacsi
 * 
 * @param str  retazec do ktoreho sa pridava
 * @param z    pridavany znak
 * @return     true = OK, false = chyba alokacie
 */
bool dynstr_append(DynStr *str, char z);

/**
 * vyprazdni retazec, pridelena pamat ostane (da sa znovu pouzit)
 * 
 * @param str  retazec ktory sa vyprazdni
 */
void dynstr_clear(DynStr *str);

/**
 * Uvolni pamat retazca
 * 
 * @param str  retazec, ktory sa uvolni
 */
void dynstr_free(DynStr *str);
#endif