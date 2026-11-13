/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Subor:   dynstr.c
 * Popis:   Implementacia dynamickeho retazca
 *
 * Autor:   Filip Slovak (xslovaf00)
 */

#include "dynstr.h"
#include <stdlib.h> // malloc, realloc, free
#define DYNSTR_INIT_CAPACITY 16

bool dynstr_init(DynStr *str) {
  str->data = malloc(DYNSTR_INIT_CAPACITY * sizeof(char));
  if (str->data == NULL) {
    return false;
  }
  str->data[0] = '\0';
  str->length = 0;
  str->capacity = DYNSTR_INIT_CAPACITY;
  return true;
}

bool dynstr_append(DynStr *str, char z) {
  if (str->length + 1 >=
      str->capacity) { // kontrola, ci je potrebne zvacsit kapacitu
    size_t new_capacity = str->capacity * 2; // zvacsime kapacitu na dvojnasobok
    char *new_data = realloc(str->data, new_capacity * sizeof(char));
    if (new_data == NULL) {
      return false; // ak sa nepodarilo alokovat pamat, vratime false
    }
    str->data = new_data;
    str->capacity = new_capacity;
  }
  str->data[str->length] = z; // pridame znak na koniec retazca
  str->length++;
  str->data[str->length] = '\0'; // pridame null terminator
  return true;
}

void dynstr_clear(DynStr *str) {
  str->length = 0;
  if (str->data != NULL) {
    str->data[0] = '\0'; // nastavime prvy znak na null terminator
  }
}

void dynstr_free(DynStr *str) {
  free(str->data);
  str->data = NULL;
  str->length = 0;
  str->capacity = 0;
}
