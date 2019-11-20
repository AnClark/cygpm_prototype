#include "lex.export.h"             // Include CPP-exported lex.yy.c definitions
#include "tokens.h"
#include <cstdio>

int numPackages = 0;
int quote = 0;

int main()
{
    int token_type;

    while (token_type = yylex())
    {
        switch(token_type) {
            case T_Package_Name:
                printf("====================================================\n");
                printf("Get package: %s\n", yytext);
                numPackages++;
                break;

            case T_YAML_Key:
                printf("Get YAML key: %s\n", yytext);
                break;

            case T_Quotation_Mark:
                // TODO: Quotation marks attached before/after a word haven't been checked.
                //      However, to all intents, it doesn't affect splitting.
                if(!quote) { 
                    printf("Quotation start\n"); 
                    quote = 1;
                } else {
                    printf("Quotation end\n"); 
                    quote = 0;
                }
                break;

            case T_Prev_Version_Mark:
                printf("* Get a previous version\n");
                break;

            case T_Word:
                printf("WORD: %s\n", yytext);
                break;
                
        }
    }
          

    printf("\nPackage count: %d\n", numPackages);
}


int yywrap()
{
    return 1;
}

