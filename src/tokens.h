#ifndef TOKENS_H
#define TOKENS_H

/**
 * Token type IDs for possible setup.ini tokens.
 */

typedef enum {
    T_Package_Name = 256,       
    T_Quotation_Mark,
    T_YAML_Key,
    T_Prev_Version_Mark,
    T_Multiline_String,
    T_Word,
    T__Ignored
} TokenType;


#endif