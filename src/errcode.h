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

struct ReturnCheck {    // check whether function's return-stmt match type
    int lno;
    bool isReTurnStmt;
    bool hasReturnValue;

    ReturnCheck():lno(0), isReTurnStmt(false), hasReturnValue(false) {}
};

class ErrorHandler {
private:
    static inline bool isNormalChar(char c) {
        return (c == 32 || c == 33 || (40 <= c && c <= 126));
    }

public:
    static bool inEffect;
    static ofstream efs;
    static bool isprint;

    static void respond(ErrCode errCode, int lno) {
        isprint && inEffect && efs << lno << " " << char(errCode) << endl;
    }

    static bool checkFormatString(string& str, int& cnt) {
        for (int i = 1; i < str.size() - 1; ++i) {
            if (str[i] == '\\') {
                if (i+1 >= str.size() || str[i+1] != 'n') {
                    return false;
                }
            } else if (str[i] == '%') {
                if (i+1 >= str.size() || str[i+1] != 'd') {
                    return false;
                } else {
                    cnt += 1;
                }
            } else if (!isNormalChar(str[i])) {
                return false;
            }
        }
        return true;
    }
};

#endif //COMPILER_ERRCODE_H
