/**
 * lex.export.h  //  Export symbols to be used by C++.
 * 
 * If you link C-compiled object/library to C++, you must export them separately
 * by using `extern "C"`.
 */

#ifndef LEX_EXPORT_H
#define LEX_EXPORT_H
#ifdef __cplusplus
extern "C"
{
    int yywrap ( void );
    int yylex( void );
    int yyleng;
    //int yylineno;
    char *yytext;
}
#endif
#endif