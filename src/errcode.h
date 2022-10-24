//
// Created by Excalibur on 2022/10/7.
//

#ifndef COMPILER_ERRCODE_H
#define COMPILER_ERRCODE_H

#include <fstream>

using namespace std;

enum struct ErrCode {
    INVALID_FSTRING = 'a',

    REDEFINE_IDENT,
    UNDEFINE_IDENT,

    UNMATCHED_FPARAM_NUM,
    UNMATCHED_FPARAM_TYPE,

    VOID_FUNC_RETURN_INT,
    INT_FUNC_RETURN_VOID,

    ASSIGN_UNMODIFIABLE_LVAL,

    LACK_SEMICOLON,
    LACK_R_PARENT,
    LACK_R_BRACK,

    UNMATCHED_FORMAT_CHAR,
    BREAK_CONTINUE_OUTLOOP,

    UNDEFINE_ERROR
};

class ErrorHandler {
public:
    static ofstream efs;
    static void respond(ErrCode errCode, int lno) {
        efs << lno << " " << char(errCode) << endl;
    }
};

#endif //COMPILER_ERRCODE_H
