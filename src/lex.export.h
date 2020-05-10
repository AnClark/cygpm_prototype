/**
 * lex.export.h  //  Export symbols to be used by C++.
 * 
 * If you link C-compiled object/library to C++, you must export their symbols separately
 * by using `extern "C"`. For variables, use "extern" in addition.
 */

#ifndef LEX_EXPORT_H
#define LEX_EXPORT_H
#ifdef __cplusplus
extern "C"
{
    int yylex(void);                  // The core function of lexer. Parse at a time from current position.
    extern char *yytext;              // Current matched text
    extern int yyleng;                // yytext's length
    extern int yylineno;              // Current line number
    void yyrestart(FILE *input_file); // Redirect the lexer's input stream
    int yywrap(void);                 // Inform if the lexer should parse more than one file.
                                      // Return 1 if only parse one.
}
#endif
#endif
