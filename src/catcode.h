//
// Created by Excalibur on 2022/9/13.
//

#ifndef COMPILER_CATCODE_H
#define COMPILER_CATCODE_H

#include <string>

using namespace std;

enum struct CatCode {
    EOL = 0,

    IDENFR,

    INT_CON,
    STR_CON,

    MAIN_TK,
    CONST_TK,
    INT_TK,
    BREAK_TK,
    CONTINUE_TK,
    IF_TK,
    ELSE_TK,

    WHILE_TK,
    GETINT_TK,
    PRINTF_TK,
    RETURN_TK,
    VOID_TK,

    NOT,
    AND,
    OR,

    PLUS,
    MINU,
    MULT,
    DIV,
    MOD,
    LSS,
    LEQ,
    GRE,
    GEQ,
    EQL,
    NEQ,

    ASSIGN,
    SEMICN,
    COMMA,
    L_PARENT,
    R_PARENT,
    L_BRACK,
    R_BRACK,
    L_BRACE,
    R_BRACE
};

string getTypeStr(CatCode type);

#endif //COMPILER_CATCODE_H
