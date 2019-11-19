#ifndef TOKENS_H
#define TOKENS_H

/**
 * Token type IDs for possible setup.ini tokens.
 */

enum TokenType {
    T_Package_Name = 256,       
    T_Quotation_Mark,
    T_YAML_Key,
    T_Prev_Version_Mark,
    T_Word,
    T__Ignored
};


#endif