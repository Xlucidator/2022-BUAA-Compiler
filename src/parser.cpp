//
// Created by Excalibur on 2022/9/20.
//

#include "parser.h"
#include "catcode.h"
#include "symbol.h"
#include "errcode.h"
#include <set>
#include <iostream>

using namespace std;

unsigned int no_cnt = 0;
SymbolTable* curContext;

ofstream ErrorHandler::efs("error.txt", ios::out);  // NOLINT
bool     ErrorHandler::inEffect = true;


void doSyntaxAnalysis() {
    if (wordList.empty())
        throw "no word to analysis!";

    SymbolTable globalTable;
    curContext = &globalTable;

    Parser parser(wordList);
    try {
        parser.parseCompUnit();
    } catch (string& err_str) {
        cout << err_str << endl;
    }
}


/*========================= class Parser =========================*/
Parser::Parser(vector<Word> &w): wordsList(w), cnt(1), peek(w[0]) {
    if (isprint) {
        ofs.open("output.txt", ios::out);
        if (ofs.fail()) {
            cerr << "failed to write!" << endl;
            return;
        }
    }
}

Parser::~Parser() {
    ofs.close();
}

void Parser::parseCompUnit() {
    while (preLook(1).type != CatCode::MAIN_TK &&
           preLook(2).type != CatCode::L_PARENT ) { // {Decl}
        if (peek.type == CatCode::CONST_TK)
            parseConstDecl();
        else
            parseVarDecl();
    }
    while (preLook(1).type != CatCode::MAIN_TK &&
           preLook(2).type == CatCode::L_PARENT) { // {FuncDef}
        parseFuncDef();
    }
    parseMainFunc(); // MainFuncDef
    genOutput("<CompUnit>");
}

void Parser::parseMainFunc() {
    vector<Param> params;

    if (peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack int";
    if (nextWord().type != CatCode::MAIN_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack main";
    if (nextWord().type != CatCode::L_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack (";
    if (nextWord().type != CatCode::R_PARENT) {
        /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack )"; */
        ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);
    } else {
        nextWord();
    }
    parseBlock(true, params);
    genOutput("<MainFuncDef>");
}

void Parser::parseBlock(bool needReturnValue, vector<Param>& funcParams) {  // init function block
    if (peek.type != CatCode::L_BRACE)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] block: not a {";

    /* enter a block: switch to a new context */
    auto funcScope = new SymbolTable(curContext, funcParams, needReturnValue);
    curContext = funcScope;

    ReturnCheck returnCheck;
    nextWord();
    while (peek.type != CatCode::R_BRACE) {
        returnCheck = parseBlockItem();
        if (!needReturnValue && returnCheck.hasReturnValue) {
            ErrorHandler::respond(ErrCode::VOID_FUNC_RETURN_INT, returnCheck.lno);          // Error: f
        }
    }
    if (needReturnValue && !returnCheck.isReTurnStmt) {
        ErrorHandler::respond(ErrCode::INT_FUNC_RETURN_VOID, peek.lno);                     // Error: g
    }
    nextWord();
    genOutput("<Block>");

    /* exit a block: switch back to previous context */
    curContext = curContext->getPreContext();
    delete funcScope;
}

void Parser::parseBlock(bool inLoop) {  // generate normal block and so on
    if (peek.type != CatCode::L_BRACE)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] block: not a {";

    /* enter a block: switch to a new context */
    auto blockScope = new SymbolTable(curContext, inLoop);
    curContext = blockScope;

    ReturnCheck returnCheck;
    nextWord();
    while (peek.type != CatCode::R_BRACE) {
        returnCheck = parseBlockItem();
        if (!curContext->isNeedReturn() && returnCheck.hasReturnValue) {
            ErrorHandler::respond(ErrCode::VOID_FUNC_RETURN_INT, returnCheck.lno);
        }
    }
    nextWord();
    genOutput("<Block>");

    /* exit a block: switch back to previous context */
    curContext = curContext->getPreContext();
    delete blockScope;
}

ReturnCheck Parser::parseBlockItem() {
    switch(peek.type) {
        // Decl
        case CatCode::CONST_TK :
            parseConstDecl();
            break;
        case CatCode::INT_TK:
            parseVarDecl();
            break;

        // Stmt
        default:
            return parseStmt(false);
    }
    return ReturnCheck{};
}

ReturnCheck Parser::parseStmt(bool inLoop) {
    ReturnCheck returnCheck;
    switch (peek.type) {
        // 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
        case CatCode::IF_TK :
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] if: lack (";
            nextWord(); parseCond();
            if (peek.type != CatCode::R_PARENT) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] if: lack )"; */
                ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);             // Error: j
            } else {
                nextWord();
            }
            parseStmt(false);
            if (peek.type == CatCode::ELSE_TK) {
                nextWord(); parseStmt(false);
            }
            break;

        // 'while' '(' Cond ')' Stmt
        case CatCode::WHILE_TK :
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] while: lack (";
            nextWord();
            parseCond();
            if (peek.type != CatCode::R_PARENT) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] while: lack )"; */
                ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);             // Error: j
            } else {
                nextWord();
            }
            parseStmt(true);
            break;

        // 'break' ';' | 'continue' ';'
        case CatCode::BREAK_TK :
        case CatCode::CONTINUE_TK :
            if (!curContext->isInLoop()) {
                ErrorHandler::respond(ErrCode::BREAK_CONTINUE_OUTLOOP, peek.lno);           // Error: m
            }
            if (nextWord().type != CatCode::SEMICN) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] break/continue: lack ;"; */
                ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);            // Error: i
            } else {
                nextWord();
            }
            break;

        // 'return' [Exp] ';'
        case CatCode::RETURN_TK:
            returnCheck.lno = peek.lno;
            returnCheck.isReTurnStmt = true;
            if (nextWord().type != CatCode::SEMICN) {
                /* detect whether there is an Exp after return, or just lack ; */
                bool hasExp = true;
                snapshot();
                    try {
                        parseExp();
                    } catch (string& err_str) {
                        hasExp = false;
                    }
                recover();

                if (hasExp) {
                    returnCheck.hasReturnValue = true;
                    parseExp();
                }
            }
            if (peek.type != CatCode::SEMICN) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] return: lack ;"; */
                ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);            // Error: i
            } else {
                nextWord();
            }
            break;

        // 'printf''('FormatString{','Exp}')'';'
        case CatCode::PRINTF_TK: {
            int printf_lno = peek.lno;
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack (";
            if (nextWord().type != CatCode::STR_CON)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack format-string";

            int format_char_cnt = 0;
            if (!ErrorHandler::checkFormatString(peek.cont, format_char_cnt)) {
                ErrorHandler::respond(ErrCode::INVALID_FSTRING, printf_lno);
            }
            nextWord();

            int exp_cnt = 0;
            while (peek.type == CatCode::COMMA) {
                nextWord();
                parseExp();
                exp_cnt += 1;
            }
            if (format_char_cnt != exp_cnt) {
                ErrorHandler::respond(ErrCode::UNMATCHED_FORMAT_CHAR, printf_lno);
            }

            if (peek.type != CatCode::R_PARENT) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack )"; */
                ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);             // Error: j
            } else {
                nextWord();
            }
            if (peek.type != CatCode::SEMICN) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack ;"; */
                ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);            // Error: i
            } else {
                nextWord();
            }
            break;
        }

        // Block
        case CatCode::L_BRACE:
            parseBlock(inLoop);
            break;

        // ';'
        case CatCode::SEMICN :
            nextWord();
            break;
        // Exp ';'
        case CatCode::PLUS: // UnaryOp: '+'
        case CatCode::MINU: // UnaryOp: '-'
        case CatCode::NOT : // UnaryOp: '!'
        case CatCode::L_PARENT: // PrimaryExp: '(' Exp ')'
        case CatCode::INT_CON : // Number: IntConst
            parseExp();
            if (peek.type != CatCode::SEMICN) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] Exp: lack ;"; */
                ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);            // Error: i
            } else {
                nextWord();
            }
            break;
        // Exp ';' / LVal '=' xxx ';'
        case CatCode::IDENFR:
            if (preLook(1).type == CatCode::L_PARENT) // Exp ';' (Ident() function)
                parseExp();
            else {
                /* detect whether it's 'LVal =' or 'Exp;' */
                bool lval_assign;
                snapshot();
                    parseLVal(nullptr);
                    lval_assign = (peek.type == CatCode::ASSIGN);
                recover();

                if (lval_assign) { // LVal '='
                    /* check is modifiable */
                    IdentItem* identItem = parseLVal(nullptr);
                    if (identItem != nullptr && !identItem->modifiable) {
                        ErrorHandler::respond(ErrCode::ASSIGN_UNMODIFIABLE_LVAL, preLook(-1).lno);  // Error: h
                    }

                    if (nextWord().type == CatCode::GETINT_TK) {    // LVal '=' 'getint' '(' ')' ';'
                        if (nextWord().type != CatCode::L_PARENT)
                            throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal = getint: lack (";
                        if (nextWord().type != CatCode::R_PARENT) {
                            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal = getint: lack )"; */
                            ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno); // Error: j
                        } else {
                            nextWord();
                        }
                    } else {                                        // LVal '=' 'Exp' ';'
                        parseExp();
                    }
                } else {            // Exp ';'
                    parseExp();
                }
            }
            /* deal with ';' together */
            if (peek.type != CatCode::SEMICN) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] Exp / LVal= : lack ;"; */
                ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);            // Error: i
            } else {
                nextWord();
            }
            break;

        default:
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized Stmt";
    }
    genOutput("<Stmt>");
    return returnCheck;
}

void Parser::parseCond() {
    parseLOrExp();
    genOutput("<Cond>");
}

Param Parser::parseExp() {
    Param param;
    param = parseAddExp();
    genOutput("<Exp>");
    return param;
}

void Parser::parseConstExp() {
    parseAddExp();
    genOutput("<ConstExp>");
}

Param Parser::parseUnaryExp() { // PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
    Param param;
    switch (peek.type) {
        // UnaryOp UnaryExp
        case CatCode::PLUS:
        case CatCode::MINU:
        case CatCode::NOT :
            parseUnaryOp();
            param = parseUnaryExp();
            break;

        // Ident
        case CatCode::IDENFR:
            if (preLook(1).type == CatCode::L_PARENT) { // Ident '(' [FuncRParams] ')'
                FuncItem* funcItem = curContext->getFunc(peek.cont);
                int func_lno = peek.lno;
                if (funcItem == nullptr) {
                    ErrorHandler::respond(ErrCode::UNDEFINE_IDENT, peek.lno);               // Error: c
                } else {
                    param.type = funcItem->type; // in fact, it'll absolutely be INT
                }

                if (nextWord().type !=CatCode::L_PARENT)
                    throw "[" + to_string(peek.lno) + " " + peek.cont + "] function call: lack (";

                vector<Param> FParams;
                if (funcItem != nullptr) {
                    FParams = funcItem->params;
                }

                vector<Param> RParams;
                if (nextWord().type != CatCode::R_PARENT) {
                    /* detect whether it's a FuncRParams, or just lack ')' */
                    bool hasFuncRParams = true;
                    snapshot();
                        try {
                            parseFuncRParams();
                        } catch (string& err_string) {
                            hasFuncRParams = false;
                        }
                    recover();

                    if (hasFuncRParams) {   // has a FuncRParams
                        RParams = parseFuncRParams();
                        if (peek.type != CatCode::R_PARENT) {
                            ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno); // Error: j
                        } else {
                            nextWord();
                        }
                    } else { // has no FuncRParams but lack ')'
                        ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);     // Error: j
                    }
                } else { // has no FuncRParams and has ')'
                    nextWord();
                }

                /* check whether function params' number & type is matched */
                if (FParams.size() != RParams.size()) {
                    ErrorHandler::respond(ErrCode::UNMATCHED_FPARAM_NUM, func_lno);
                } else {
                    for (int i = 0; i < FParams.size(); ++i) {
                        if (FParams[i].dim.size() != RParams[i].dim.size()) {
                            ErrorHandler::respond(ErrCode::UNMATCHED_FPARAM_TYPE, func_lno);
                            break;
                        }
                    }
                }
            } else {  // PrimaryExp
                param = parsePrimaryExp();
            }
            break;

        case CatCode::L_PARENT:
        case CatCode::INT_CON :
            param = parsePrimaryExp();
            break;

        default:
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized UnaryExp";
    }
    genOutput("<UnaryExp>");
    return param;
}

IdentItem* Parser::parseLVal(vector<int>* offsets) { // Ident {'[' Exp ']'}
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal: lack Ident";
    IdentItem* identItem = curContext->getIdent(peek.cont);
    if (identItem == nullptr) {
        ErrorHandler::respond(ErrCode::UNDEFINE_IDENT, peek.lno);                           // Error: c
    }
    nextWord();
    while (peek.type == CatCode::L_BRACK) {
        nextWord(); parseExp();
        if (offsets != nullptr) offsets->push_back(0);
        if (peek.type != CatCode::R_BRACK) {
            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal: array lack ]"; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, preLook(-1).lno);                  // Error: k
        } else {                              /* in case "a[3][2" with no ']' and ';' */
            nextWord();
        }
    }
    genOutput("<LVal>");
    return identItem;
}

Param Parser::parsePrimaryExp() { // '(' Exp ')' | LVal | Number
    Param param;
    IdentItem* identItem;
    vector<int> off_selects;    // fake now
    switch (peek.type) {
        // '(' Exp ')'
        case CatCode::L_PARENT :
            nextWord();
            param = parseExp();
            if (peek.type != CatCode::R_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] PrimaryExp-(Exp): lack )";
            nextWord();
            break;

        // Number
        case CatCode::INT_CON :
            param.type = Type::INT;
            parseNumber();
            break;

        // LVal
        case CatCode::IDENFR :
            identItem = parseLVal(&off_selects);
            if (identItem != nullptr) {
                param.type = identItem->type;
                for (auto i = off_selects.size(); i < identItem->dim.size(); ++i)
                    param.dim.push_back(identItem->dim[i]);
            }
            break;

        default:
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized PrimaryExp";
    }
    genOutput("<PrimaryExp>");
    return param;  //
}

void Parser::parseConstDecl() { // 'const' BType ConstDef { ',' ConstDef } ';'
    if (peek.type != CatCode::CONST_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack const\n"; // unnecessary
    if (nextWord().type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack BType(int)";
    do {
        nextWord(); parseConstDef();
    } while (peek.type == CatCode::COMMA);
    if (peek.type != CatCode::SEMICN) {
        /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack ;"; */
        ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);                    // Error: i
    } else {
        nextWord();
    }
    genOutput("<ConstDecl>");
}

void Parser::parseConstDef() { // Ident { '[' ConstExp ']' } '=' ConstInitVal
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: lack Ident";
    IdentItem* identItem = curContext->addIdent(peek, false);
    while (nextWord().type == CatCode::L_BRACK) {
        nextWord(); parseConstExp();
        if (identItem != nullptr) identItem->dim.push_back(0);  // update dim
        if (peek.type != CatCode::R_BRACK) {
            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: array lack ] to match ["; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, peek.lno);
            prevWord();
        }
    }
    if (peek.type != CatCode::ASSIGN)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: lack =";
    nextWord(); parseConstInitVal();
    genOutput("<ConstDef>");
}

void Parser::parseConstInitVal() { // ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
    if (peek.type == CatCode::L_BRACE) {
        if (nextWord().type != CatCode::R_BRACE) { // [ ConstInitVal { ',' ConstInitVal } ] '}'
            parseConstInitVal();
            while (peek.type == CatCode::COMMA) {
                nextWord(); parseConstInitVal();
            }
            if (peek.type != CatCode::R_BRACE)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstInitVal: lack";
        }
        nextWord();
    } else {
        parseConstExp();
    }
    genOutput("<ConstInitVal>");
}

void Parser::parseVarDecl() { // BType VarDef { ',' VarDef } ';'
    if (peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDecl: lack BType(int)";
    do {
        nextWord(); parseVarDef();
    } while (peek.type == CatCode::COMMA);
    if (peek.type != CatCode::SEMICN) {
        /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDecl: lack ;"; */
        ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);                    // Error: i
    } else {
        nextWord();
    }
    genOutput("<VarDecl>");
}

void Parser::parseVarDef() { // Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDef: lack Ident";
    IdentItem* identItem = curContext->addIdent(peek, true);
    while (nextWord().type == CatCode::L_BRACK) {
        nextWord(); parseConstExp();
        if (identItem != nullptr) identItem->dim.push_back(0);  // maybe we should wrapped it to an api
        if (peek.type != CatCode::R_BRACK) {
            /*throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDef: array lack ] to match ["; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, peek.lno);                         // Error: k
            prevWord();
        }
    }
    if (peek.type == CatCode::ASSIGN) { // ... '=' InitVal
        nextWord(); parseInitVal();
    }
    genOutput("<VarDef>");
}

void Parser::parseInitVal() { // Exp | '{' [ InitVal { ',' InitVal } ] '}'
    if (peek.type == CatCode::L_BRACE) {
        if (nextWord().type != CatCode::R_BRACE) { // [ InitVal { ',' InitVal } ] '}'
            parseInitVal();
            while (peek.type == CatCode::COMMA) {
                nextWord(); parseInitVal();
            }
            if (peek.type != CatCode::R_BRACE)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] InitVal: lack";
        }
        nextWord();
    } else {
        parseExp();
    }
    genOutput("<InitVal>");
}

void Parser::parseFuncDef() {
    /* get function type */
    Type ftype = parseFuncType();

    /* get function name & lno */
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack Ident";
    string fname = peek.cont;
    int f_lno = peek.lno;

    /* get function parameters */
    vector<Param> params;
    if (nextWord().type != CatCode::L_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack (";
    if (nextWord().type != CatCode::R_PARENT) {
        params = parseFuncFParams();
    }

    curContext->addFunc(fname, ftype, params, f_lno);

    if (peek.type != CatCode::R_PARENT) {
        /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack )"; */
        ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);                     // Error: j
    } else {
        nextWord();
    }
    parseBlock(ftype != Type::VOID, params);
    genOutput("<FuncDef>");
}

vector<Param> Parser::parseFuncFParams() {
    vector<Param> params;
    set<string> para_names;  /* seemed ugly: should be dealt in FuncItem */

    params.push_back(parseFuncFParam());
    para_names.insert(params[0].name);
    while (peek.type == CatCode::COMMA) {
        nextWord();
        Param param = parseFuncFParam();
        if (para_names.find(param.name) != para_names.end()) {
            ErrorHandler::respond(ErrCode::REDEFINE_IDENT, preLook(-1).lno);                // Error: b
        } else {
            params.push_back(param);
            para_names.insert(param.name);
        }
    }
    genOutput("<FuncFParams>");
    return params;
}

Param Parser::parseFuncFParam() { // BType Ident ['[' ']' { '[' ConstExp ']' }]
    Param param;

    /* get parameter(ident) type */
    param.type = parseBType();

    /* get parameter(ident) name */
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncFParam: lack Ident";
    param.name = peek.cont;

    /* get parameter(ident) dim */
    if (nextWord().type == CatCode::L_BRACK) { // '[' ']' { '[' ConstExp ']' }
        if (nextWord().type != CatCode::R_BRACK) {
            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "]
             * FuncFParam: array param lack ] to match ["; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, peek.lno);                         // Error: k
            prevWord();
        }
        param.dim.push_back(0);
        while (nextWord().type == CatCode::L_BRACK) { // { '[' ConstExp ']' }
            nextWord(); parseConstExp();
            param.dim.push_back(0); // value from parseConstExp()
            if (peek.type != CatCode::R_BRACK) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "]/
                 * FuncFParam: mul arrays param lack ] to match ["; */
                ErrorHandler::respond(ErrCode::LACK_R_BRACK, peek.lno);                     // Error: k
                prevWord();
            }
        }
    }
    genOutput("<FuncFParam>");
    return param;
}

vector<Param> Parser::parseFuncRParams() {
    vector<Param> params;
    params.push_back(parseExp());
    while (peek.type == CatCode::COMMA) {
        nextWord();
        params.push_back(parseExp());
    }
    genOutput("<FuncRParams>");
    return params;
}