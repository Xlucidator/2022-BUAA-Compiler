//
// Created by Excalibur on 2022/9/13.
//

#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H


#include "catcode.h"
#include "../settings.h"

#include <string>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

#define S_INIT  1
#define S_MANN  2   // no need for LANN (line annotation)
#define S_IDENT 4
#define S_STR   5
#define S_NUM   6
#define S_DDB   7
#define S_END   8


void doLexicalAnalysis();
void printWordList();
void fprintWordList();


class Lexer {

private:
    int state;
    int pos;  // current pos in the line
    bool eol;
    int lno;  // current line of program
    string str;
    const string dsg = "+-*/%;,()[]{}"; // surely be single-divider
    const string ddb = "&|<>=!";        // may lead to double-divider

public:
    static bool isprint;
    const map<string, CatCode> token2Catcode = {
            {"main"    , CatCode::MAIN_TK    },
            {"const"   , CatCode::CONST_TK   },
            {"int"     , CatCode::INT_TK     },
            {"break"   , CatCode::BREAK_TK   },
            {"continue", CatCode::CONTINUE_TK},
            {"if"      , CatCode::IF_TK      },
            {"else"    , CatCode::ELSE_TK    },
            {"while"   , CatCode::WHILE_TK   },
            {"getint"  , CatCode::GETINT_TK  },
            {"printf"  , CatCode::PRINTF_TK  },
            {"return"  , CatCode::RETURN_TK  },
            {"void"    , CatCode::VOID_TK    },
            {"+"  , CatCode::PLUS    },
            {"-"  , CatCode::MINU    },
            {"*"  , CatCode::MULT    },
            {"/"  , CatCode::DIV     },
            {"%"  , CatCode::MOD     },
            {";"  , CatCode::SEMICN  },
            {","  , CatCode::COMMA   },
            {"("  , CatCode::L_PARENT},
            {")"  , CatCode::R_PARENT},
            {"["  , CatCode::L_BRACK },
            {"]"  , CatCode::R_BRACK },
            {"{"  , CatCode::L_BRACE },
            {"}"  , CatCode::R_BRACE },
            {"&&" , CatCode::AND     },
            {"||" , CatCode::OR      },
            {"<"  , CatCode::LSS     },
            {"<=" , CatCode::LEQ     },
            {">"  , CatCode::GRE     },
            {">=" , CatCode::GEQ     },
            {"="  , CatCode::ASSIGN  },
            {"==" , CatCode::EQL     },
            {"!"  , CatCode::NOT     },
            {"!=" , CatCode::NEQ     }
    };
    const map<CatCode, string> Catcode2print = {
            {CatCode::IDENFR      , "IDENFR"    },
            {CatCode::INT_CON     , "INTCON"    },
            {CatCode::STR_CON     , "STRCON"    },
            {CatCode::MAIN_TK     , "MAINTK"    },
            {CatCode::CONST_TK    , "CONSTTK"   },
            {CatCode::INT_TK      , "INTTK"     },
            {CatCode::BREAK_TK    , "BREAKTK"   },
            {CatCode::CONTINUE_TK , "CONTINUETK"},
            {CatCode::IF_TK       , "IFTK"      },
            {CatCode::ELSE_TK     , "ELSETK"    },
            {CatCode::NOT         , "NOT"       },
            {CatCode::AND         , "AND"       },
            {CatCode::OR          , "OR"        },
            {CatCode::WHILE_TK    , "WHILETK"   },
            {CatCode::GETINT_TK   , "GETINTTK"  },
            {CatCode::PRINTF_TK   , "PRINTFTK"  },
            {CatCode::RETURN_TK   , "RETURNTK"  },
            {CatCode::PLUS        , "PLUS"      },
            {CatCode::MINU        , "MINU"      },
            {CatCode::VOID_TK     , "VOIDTK"    },
            {CatCode::MULT        , "MULT"      },
            {CatCode::DIV         , "DIV"       },
            {CatCode::MOD         , "MOD"       },
            {CatCode::LSS         , "LSS"       },
            {CatCode::LEQ         , "LEQ"       },
            {CatCode::GRE         , "GRE"       },
            {CatCode::GEQ         , "GEQ"       },
            {CatCode::EQL         , "EQL"       },
            {CatCode::NEQ         , "NEQ"       },
            {CatCode::ASSIGN      , "ASSIGN"    },
            {CatCode::SEMICN      , "SEMICN"    },
            {CatCode::COMMA       , "COMMA"     },
            {CatCode::L_PARENT    , "LPARENT"   },
            {CatCode::R_PARENT    , "RPARENT"   },
            {CatCode::L_BRACK     , "LBRACK"    }
    };

public:
    Lexer();
    void refillStr(string& input);
    void lexing();
    string nextToken();
    CatCode getWordType(string& token);
};

struct Word {
    CatCode type;
    string cont;
    int lno;
    Word(): type(), cont() {}
    explicit Word(CatCode c): type(c), cont() {}
};

extern vector<Word> wordList;

#endif //COMPILER_LEXER_H
