%{
//#include "lex.export.h"
#include "tokens.h"
%}

WORD                ([^ \t\n\r\a]+)
PACKAGE_NAME        ("@"[ ][0-9A-Za-z\-+_]+)
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

^{PACKAGE_NAME}     { return T_Package_Name;                              }
"[prev]"            { return T_Prev_Version_Mark;                         }
\"                  { return T_Quotation_Mark;                            }
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

int yywrap()
{
    return 1;
}