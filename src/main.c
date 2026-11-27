/*
 * Projekt: Implementace prekladace imperativniho jazyka IFJ26
 * Subor:   main.c
 * Popis:   Hlavny program prekladaca, spaja jednotlive casti prekladu
 *
 * Autor:   Filip Slovak (xslovaf00)
 */

#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symtable.h"

#include <stdio.h>

/*
 * Prekladac cita zdrojovy program v IFJ26 zo stdin a na stdout vypise
 * program v IFJcode26. Chybove hlasky idu na stderr.
 *
 * Postup prekladu:
 *  1. lexikalna analyza  - lexer (lexer.c) cita znaky a vytvara tokeny,
 *  2. syntakticka analyza - parser (parser.c, expr.c) si pyta tokeny
 *                           od lexera a stavia abstraktny syntakticky strom,
 *  3. semanticka analyza  - kontrola typov a definicii nad stromom,
 *  4. generovanie kodu    - prevod stromu do IFJcode26.
 *
 * Navratove kody (podla zadania):
 *  0 = preklad bez chyby, 1 = lexikalna chyba, 2 = syntakticka chyba,
 *  3 az 7 a 10 = semanticke chyby, 99 = interna chyba (napr. malloc).
 */
int main(void)
{
	// 1. Pripravime lexer na citanie zdrojoveho programu zo stdin
	int err = lexer_init(stdin);
	if (err != 0)
	{
		return err;
	}

	// 2. Syntakticka analyza. Parser si sam vypytuje tokeny od lexera.
	//    Pri lexikalnej (1) alebo syntaktickej (2) chybe a pri chybe
	//    pamate (99) parser ukonci program sam s prislusnym kodom.
	AstNode* program = parse_program();

	// 3. Semanticka analyza: kontrola typov a definicii nad stromom.
	//    Pri chybe vratime jej navratovy kod (3 az 7 alebo 10).
	AVLNode* symtable;
	table_inicialization(&symtable);
	err = semantic_analyze(program, &symtable);
	if (err != 0)
	{
		table_free(&symtable);
		ast_free(program);
		lexer_free();
		return err;
	}

	// 4. Generovanie IFJcode26 na stdout
	codegen_generate(program);

	// 5. Uvolnime vsetku pamat
	table_free(&symtable);
	ast_free(program);
	lexer_free();
	return 0;
}
