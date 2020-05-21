%{
#include "tokens.h"

char* str_preprocessor(char *src);
%}

WORD                ([^ \t\n\r\a]+)
PACKAGE_NAME        ("@"[ ][0-9A-Za-z\-+_.]+)
YAML_KEY            ([_A-Za-z0-9\-]+":")
SINGLE_COMMENT      ("#"[^\n]*)
MULTILINE_STRING    (["][^"]*["])

%%
\n                  { return T__Ignored;     /** Trailing endlines **/    }
^{SINGLE_COMMENT}   { /** Skip for single-line omments **/                
                      /** TODO:
                        - Support inline comments
                        - Allow "#" mark within quotation mark domain
                      **/
                    }

^{PACKAGE_NAME}     { return T_Package_Name;                              }
"[prev]"            { return T_Prev_Version_Mark;                         }
{MULTILINE_STRING}  { /* yytext = str_preprocessor(yytext); */  return T_Multiline_String;     }
^{YAML_KEY}         { return T_YAML_Key;                                  }
{WORD}              { return T_Word;                                      }

.                   { return T__Ignored;     /** Any other tokens **/     }
<<EOF>>             { return 0;                                           }

%%

/** 
 * Export symbols for C++ 
 * Must be put at the end of the lexer file, or they'll be overridden.
*/
#include "lex.export.h"

/**
 * Preprocess strings for SQLite.
 * By default, SQLite doesn't support "'" in queries.
 * UPDATE: No need to preprocess if you're using SQLite statement binding.
 *         SQLite will process them automatically.
 */
char* str_preprocessor(char *src)
{
    // Calculate how much single quotation marks are there
    int i, j;
    int count_single_quot_mark = 0;
    for (i = 0; i < strlen(src); i++)
        if (src[i] == '\'')
            count_single_quot_mark++;

    // Create a new buffer for preprocessed string
    char *dest = malloc(strlen(src) + count_single_quot_mark + 1);

    // Refill destination with source
    i = 0;
    j = 0;
    for(i = 0; i < strlen(src); i++)
    {
        // Double every single quotation marks to escape it in SQL queries
        if(src[i] == '\'')
        {
            dest[j++] = '\'';
            dest[j++] = '\'';
        }
        else
        {
            dest[j++] = src[i];
        }
    }

    // Don't forget to enclose the dest string with '\0'
    dest[j] = '\0';

    return dest;
}

int yywrap()
{
    return 1;
}
