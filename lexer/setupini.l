%{
#include "tokens.h"
int numPackages = 0;
int quote = 0;
%}

WORD                ([^ \t\n\r\a]+)
PACKAGE_NAME        ("@"[ ][0-9A-Za-z\-_]+)
YAML_KEY            ([_A-Za-z0-9\-]+":")
SINGLE_COMMENT      ("#"[^\n]*)

%%
\n                  { return T__Ignored;     /** Trailing endlines **/    }
^{SINGLE_COMMENT}   { /** Skip for single-line omments **/                
                      /** TODO:
                        - Support inline comments
                        - Allow "#" mark within quotation mark domain
                      **/
                    }

^{PACKAGE_NAME}     { numPackages++;   return T_Package_Name;             }
"[prev]"            { return T_Prev_Version_Mark;                         }
\"                  { return T_Quotation_Mark;                            }
^{YAML_KEY}         { return T_YAML_Key;                                  }
{WORD}              { return T_Word;                                      }

.                   { return T__Ignored;     /** Any other tokens **/     }
<<EOF>>             { return 0;                                           }

%%

int main()
{
    int token_type;
    while (token_type = yylex())
    {
        switch(token_type) {
            case T_Package_Name:
                printf("====================================================\n");
                printf("Get package: %s\n", yytext);
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
