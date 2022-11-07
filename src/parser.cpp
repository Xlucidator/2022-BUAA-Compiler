//
// Created by Excalibur on 2022/9/20.
//

#include "tools.h"
#include "parser.h"
#include "catcode.h"
#include "symbol.h"
#include "errcode.h"
#include "irbuilder.h"

#include <set>
#include <vector>
#include <numeric>
#include <iostream>

using namespace std;

unsigned int no_cnt = 0;
SymbolTable* curContext;

ofstream ErrorHandler::efs("error.txt", ios::out);  // NOLINT
bool     ErrorHandler::inEffect = true;
bool     ErrorHandler::isprint  = ERROR_PRINT;


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

    irBuilder.printIRs();
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
    string MAIN_STR = "main";
    string INT_STR = "int";

    irBuilder.addItemDefFunc(INT_STR, MAIN_STR);

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

    irBuilder.addItemDefEnd(MAIN_STR);

    genOutput("<MainFuncDef>");
}

void Parser::parseBlock(bool needReturnValue, vector<Param>& funcParams) {  // init function block
    if (peek.type != CatCode::L_BRACE)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] block: not a {";

    /* enter a block: switch to a new context */
    auto funcScope = new SymbolTable(curContext, funcParams, needReturnValue);
    curContext = funcScope;

    // after which the params will be added to the new (function type) symbol table and curContext is switched
    // this is the exact right time to generate IROp::FPARA!
    for (auto& param : funcParams) {
        string paramName = param.name;
        string paramType = "int";
        if (!param.dim.empty()) {
            for (auto &index : param.dim) {
                if (index == 0) paramType += "[]";
                else paramType += "[" + to_string(index) + "]";
            }
        }
        irBuilder.addItemDefFParam(paramName, to_string(param.dim.size()), paramType);
    }

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
        case CatCode::RETURN_TK: {
            string GET_symbol;

            returnCheck.lno = peek.lno;
            returnCheck.isReTurnStmt = true;
            if (nextWord().type != CatCode::SEMICN) {
                /* detect whether there is an Exp after return, or just lack ; */
                bool hasExp = true;
                snapshot();
                try {
                    parseExp(nullptr);
                } catch (string &err_str) {
                    hasExp = false;
                }
                recover();

                if (hasExp) {
                    returnCheck.hasReturnValue = true;
                    parseExp(&GET_symbol);
                    irBuilder.addItemFuncReturn(GET_symbol);
                }
            }
            if (peek.type != CatCode::SEMICN) {
                /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] return: lack ;"; */
                ErrorHandler::respond(ErrCode::LACK_SEMICOLON, preLook(-1).lno);            // Error: i
            } else {
                nextWord();
            }
            break;
        }

        // 'printf''('FormatString{','Exp}')'';'
        case CatCode::PRINTF_TK: {
            int printf_lno = peek.lno;
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack (";
            if (nextWord().type != CatCode::STR_CON)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack format-string";

            int format_char_cnt = 0;
            string formatString = stripQuot(peek.cont);
            vector<string> GET_pureStrings;
            if (!ErrorHandler::checkFormatString(peek.cont, format_char_cnt)) {
                ErrorHandler::respond(ErrCode::INVALID_FSTRING, printf_lno);
            }
            GET_pureStrings = split(formatString, "%d");
            nextWord();

            int exp_cnt = 0;
            vector<string> GET_symbols;
            while (peek.type == CatCode::COMMA) {
                string GET_symbol;
                nextWord();
                parseExp(&GET_symbol);
                GET_symbols.push_back(move(GET_symbol));
                exp_cnt += 1;
            }
            if (format_char_cnt != exp_cnt) {
                ErrorHandler::respond(ErrCode::UNMATCHED_FORMAT_CHAR, printf_lno);
            }

            for (int i = 0; i < GET_pureStrings.size(); ++i) {
                if (!GET_pureStrings[i].empty()) {
                    irBuilder.addItemPrintf("\"" + GET_pureStrings[i] + "\"");
                }
                if (i < GET_symbols.size()) irBuilder.addItemPrintf(GET_symbols[i]);
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
            parseExp(nullptr);
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
                parseExp(nullptr);
            else {
                /* detect whether it's 'LVal =' or 'Exp;' */
                bool lval_assign;
                snapshot();
                    parseLVal(nullptr, nullptr, false);
                    lval_assign = (peek.type == CatCode::ASSIGN);
                recover();

                if (lval_assign) {  // LVal '=' xxx ';'
                    string GET_symbolLVal;
                    string GET_symbolRVal;

                    /* check is modifiable */
                    IdentItem* identItem = parseLVal(nullptr, &GET_symbolLVal, false);
                    if (identItem != nullptr && !identItem->modifiable) {
                        ErrorHandler::respond(ErrCode::ASSIGN_UNMODIFIABLE_LVAL, preLook(-1).lno);  // Error: h
                    }

                    if (nextWord().type == CatCode::GETINT_TK) {    // LVal '=' 'getint' '(' ')' ';'
                        GET_symbolRVal = irBuilder.addItemScanf();
                        if (nextWord().type != CatCode::L_PARENT)
                            throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal = getint: lack (";
                        if (nextWord().type != CatCode::R_PARENT) {
                            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal = getint: lack )"; */
                            ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno); // Error: j
                        } else {
                            nextWord();
                        }
                    } else {                                        // LVal '=' 'Exp' ';'
                        parseExp(&GET_symbolRVal);
                    }

                    if (isarray(GET_symbolLVal)) {
                        string ident = getArrayIdent(GET_symbolLVal);
                        string index = getArrayIndex(GET_symbolLVal);
                        irBuilder.addItemStoreArray(ident, index, GET_symbolRVal);
                    } else {
                        irBuilder.addItemAssign(GET_symbolLVal, GET_symbolRVal);
                    }
                } else {            // Exp ';'
                    parseExp(nullptr);
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

Param Parser::parseExp(string* OUT_symbol) {
    string GET_symbol;
    Param param;
    param = parseAddExp(GET_symbol);
    genOutput("<Exp>");
    if (OUT_symbol != nullptr) {
        if (hasSign(GET_symbol)) {
            removeSign(GET_symbol);
            GET_symbol = irBuilder.addItemCalculateExp(IROp::MIN, "0", GET_symbol);
        }
        (*OUT_symbol) = GET_symbol;
    }
    return param;
}

void Parser::parseConstExp(int& OUT_number) {
    string GET_symbol;
    parseAddExp(GET_symbol);
    genOutput("<ConstExp>");

    if (!isnumber(GET_symbol)) {
        OUT_number = 0;
        throw "[" + to_string(preLook(-1).lno) + "] ConstExp: must be calculable";
    } else if (hasSign(GET_symbol)) {
        OUT_number = 0;
        throw "[" + to_string(preLook(-1).lno) + "] ConstExp: can never be negative";
    }
    OUT_number = stoi(GET_symbol);
}

IdentItem* Parser::parseLVal(vector<int>* OUT_offsets, string* OUT_symbolLVal, bool IN_wrapArray) {
    // Ident {'[' Exp ']'}
    string GET_symbolLVal;
    vector<int> GET_symbolDims;

    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal: lack Ident";
    IdentItem* identItem = curContext->getIdent(peek.cont);
    if (identItem == nullptr) {
        ErrorHandler::respond(ErrCode::UNDEFINE_IDENT, peek.lno);                           // Error: c
    } else {
        GET_symbolLVal = identItem->name;
        GET_symbolDims = identItem->dim;
    }

    string index = "0";
    string GET_symbol;
    string GET_dimBase;
    int dim_cnt = 0;
    while (nextWord().type == CatCode::L_BRACK) {
        // always locate the array element
        nextWord();
        parseExp(&GET_symbol);
        if (dim_cnt < GET_symbolDims.size()) {
            int base = accumulate(GET_symbolDims.begin()+dim_cnt+1, GET_symbolDims.end(), 1, multiplies<int>());
            if (isnumber(GET_symbol)) {
                int incr = stoi(GET_symbol) * base;
                index = (isnumber(index)) ? to_string(stoi(index) + incr) :
                        irBuilder.addItemCalculateExp(IROp::ADD, index, to_string(incr));
            } else {
                string incr = irBuilder.addItemCalculateExp(IROp::MUL, GET_symbol, to_string(base));
                index = irBuilder.addItemCalculateExp(IROp::ADD, index, incr);
            }
        }
        if (OUT_offsets != nullptr)  // 0 is placeholder. in fact value is of no use
            OUT_offsets->push_back(isnumber(GET_symbol) ? stoi(GET_symbol) : 0);
        if (peek.type != CatCode::R_BRACK) {
            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal: array lack ]"; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, preLook(-1).lno);                  // Error: k
            prevWord();               /* in case "a[3][2" with no ']' and ';' in next line */
        }
        dim_cnt += 1;
    }

    if (OUT_symbolLVal != nullptr) {
        if (!GET_symbolDims.empty()) {
            if (IN_wrapArray)
                (*OUT_symbolLVal) = irBuilder.addItemLoadArray(GET_symbolLVal, index);
            else
                (*OUT_symbolLVal) = GET_symbolLVal + "[" + index + "]";
        } else {
            (*OUT_symbolLVal) = GET_symbolLVal;
        }
    }
    genOutput("<LVal>");
    return identItem;
}

Param Parser::parseUnaryExp(string& OUT_symbol) { // PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
    Param param;
    switch (peek.type) {
        // UnaryOp UnaryExp
        case CatCode::PLUS:
        case CatCode::MINU:
        case CatCode::NOT : {
            string GET_unaryOp;
            string GET_symbol;

            parseUnaryOp(GET_unaryOp);  // TODO: CatCode::NOT hasn't been dealt
            param = parseUnaryExp(GET_symbol);

            if (GET_unaryOp == "-") {
                if (hasSign(GET_symbol))
                    OUT_symbol = GET_symbol.substr(1);
                else
                    OUT_symbol = "-" + GET_symbol;
            } else {
                OUT_symbol = GET_symbol;
            }
            break;
        }

        // Ident
        case CatCode::IDENFR:
            if (preLook(1).type == CatCode::L_PARENT) { // Ident '(' [FuncRParams] ')'
                string GET_funcName = peek.cont;

                FuncItem* funcItem = curContext->getFunc(peek.cont);
                int func_lno = peek.lno;
                if (funcItem == nullptr) {
                    ErrorHandler::respond(ErrCode::UNDEFINE_IDENT, peek.lno);               // Error: c
                } else {
                    param.type = funcItem->type;
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
                    //      e.g. foo()*a;  -> foo( *a;
                    //           foo()+5;  -> foo( +5;  ambiguous
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
                        if ((FParams[i].type       != RParams[i].type) ||
                            (FParams[i].dim.size() != RParams[i].dim.size())
                            ) {
                            ErrorHandler::respond(ErrCode::UNMATCHED_FPARAM_TYPE, func_lno);
                            break;
                        }
                    }
                }

                irBuilder.addItemCallFunc(GET_funcName);
                if (param.type != Type::VOID) { // TODO: use a new IR - GET_RET
                    OUT_symbol = irBuilder.addItemCalculateExp(IROp::ADD, "RET", "0");
                } else {
                    OUT_symbol = "";
                }
            } else {  // PrimaryExp
                string GET_symbol;

                param = parsePrimaryExp(GET_symbol);
                OUT_symbol = GET_symbol;
            }
            break;

        case CatCode::L_PARENT:
        case CatCode::INT_CON : {
            string GET_symbol;

            param = parsePrimaryExp(GET_symbol);
            OUT_symbol = GET_symbol;
            break;
        }

        default:
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized UnaryExp";
    }
    genOutput("<UnaryExp>");
    return param;
}

Param Parser::parsePrimaryExp(string& OUT_symbol) { // '(' Exp ')' | LVal | Number
    Param param;

    switch (peek.type) {
        // '(' Exp ')'
        case CatCode::L_PARENT : {
            string GET_symbol;

            nextWord();
            param = parseExp(&GET_symbol);
            if (peek.type != CatCode::R_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] PrimaryExp-(Exp): lack )";
            nextWord();

            OUT_symbol = GET_symbol;
            break;
        }

        // Number
        case CatCode::INT_CON : {
            string GET_number;

            param.type = Type::INT;
            parseNumber(GET_number);

            OUT_symbol = GET_number;
            break;
        }

        // LVal
        case CatCode::IDENFR : {
            IdentItem* GET_identItem;
            vector<int> GET_offsets;    // fake now
            string GET_symbol;

            GET_identItem = parseLVal(&GET_offsets, &GET_symbol, true);
            if (GET_identItem != nullptr) {
                param.type = GET_identItem->type;
                for (auto i = GET_offsets.size(); i < GET_identItem->dim.size(); ++i)
                    param.dim.push_back(GET_identItem->dim[i]);
            }

            OUT_symbol = GET_symbol;
            break;
        }

        default:
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized PrimaryExp";
    }
    genOutput("<PrimaryExp>");
    return param;  //
}

void Parser::parseConstDecl() { // 'const' BType ConstDef { ',' ConstDef } ';'
    if (peek.type != CatCode::CONST_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack const\n"; // unnecessary
    nextWord();
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

    string PUT_ident = peek.cont;
    vector<int> PUT_dims;
    IdentItem* identItem = curContext->addIdent(peek, false);
    while (nextWord().type == CatCode::L_BRACK) { // has []
        int GET_number;
        nextWord();

        parseConstExp(GET_number);
        PUT_dims.push_back(GET_number);
        if (identItem != nullptr)
            identItem->dim.push_back(GET_number);  // update dim

        if (peek.type != CatCode::R_BRACK) {
            /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: array lack ] to match ["; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, peek.lno);                         // Error: k
            prevWord();
        }
    }
    int indexSize = accumulate(PUT_dims.begin(), PUT_dims.end(), 1, multiplies<int>());
    irBuilder.addItemDef(IROp::DEF_CON, to_string(indexSize), PUT_ident, !PUT_dims.empty());

    if (peek.type != CatCode::ASSIGN)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: lack =";
    nextWord();
    parseConstInitVal(PUT_ident, PUT_dims, 0);

    irBuilder.addItemDefEnd(PUT_ident);
    genOutput("<ConstDef>");
}

void Parser::parseConstInitVal(string IN_ident, vector<int> IN_dims, int IN_index) {
    // ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
    if (peek.type == CatCode::L_BRACE) {
        if (nextWord().type != CatCode::R_BRACE) { // [ ConstInitVal { ',' ConstInitVal } ] '}'
            IN_dims.erase(IN_dims.begin());
            int step = accumulate(IN_dims.begin(), IN_dims.end(), 1, multiplies<int>());

            parseConstInitVal(IN_ident, IN_dims, IN_index);
            IN_index += step;
            while (peek.type == CatCode::COMMA) {
                nextWord();
                parseConstInitVal(IN_ident, IN_dims, IN_index);
                IN_index += step;
            }
            if (peek.type != CatCode::R_BRACE)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstInitVal: lack }";
        }
        nextWord();
    } else {
        int GET_number;
        parseConstExp(GET_number);
        irBuilder.addItemDefInit(IN_ident, to_string(IN_index), to_string(GET_number));
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

    string PUT_ident = peek.cont;
    vector<int> PUT_dims;
    IdentItem* identItem = curContext->addIdent(peek, true);
    while (nextWord().type == CatCode::L_BRACK) {
        int GET_number;
        nextWord();

        parseConstExp(GET_number);
        PUT_dims.push_back(GET_number);
        if (identItem != nullptr)
            identItem->dim.push_back(GET_number);  // maybe we should wrapped it to an api

        if (peek.type != CatCode::R_BRACK) {
            /*throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDef: array lack ] to match ["; */
            ErrorHandler::respond(ErrCode::LACK_R_BRACK, peek.lno);                         // Error: k
            prevWord();
        }
    }
    int indexSize = accumulate(PUT_dims.begin(), PUT_dims.end(), 1, multiplies<int>());
    irBuilder.addItemDef(IROp::DEF_VAR, to_string(indexSize), PUT_ident, !PUT_dims.empty());

    if (peek.type == CatCode::ASSIGN) { // ... '=' InitVal
        nextWord();
        parseInitVal(PUT_ident, PUT_dims, 0);
    }

    irBuilder.addItemDefEnd(PUT_ident);
    genOutput("<VarDef>");
}

void Parser::parseInitVal(string IN_ident, vector<int> IN_dims, int IN_index) {
    // Exp | '{' [ InitVal { ',' InitVal } ] '}'
    if (peek.type == CatCode::L_BRACE) {
        if (nextWord().type != CatCode::R_BRACE) { // [ InitVal { ',' InitVal } ] '}'
            IN_dims.erase(IN_dims.begin());
            int step = accumulate(IN_dims.begin(), IN_dims.end(), 1, multiplies<int>());

            parseInitVal(IN_ident, IN_dims, IN_index);
            IN_index += step;
            while (peek.type == CatCode::COMMA) {
                nextWord();
                parseInitVal(IN_ident, IN_dims, IN_index);
                IN_index += step;
            }
            if (peek.type != CatCode::R_BRACE)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] InitVal: lack }";
        }
        nextWord();
    } else {
        string GET_symbol;
        parseExp(&GET_symbol);
        irBuilder.addItemDefInit(IN_ident, to_string(IN_index), move(GET_symbol));
    }
    genOutput("<InitVal>");
}

void Parser::parseFuncDef() {
    /* get function type */
    Type ftype = parseFuncType();
    string funcType = (ftype == Type::VOID) ? "void" : "int";

    /* get function name & lno */
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack Ident";
    string funcName = peek.cont;
    int f_lno = peek.lno;

    irBuilder.addItemDefFunc(funcType, funcName);

    /* get function parameters */
    vector<Param> params;
    if (nextWord().type != CatCode::L_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack (";
    if (nextWord().type != CatCode::R_PARENT) {
        /* check whether it has FuncFParams, or just lack ')' */
        if (peek.type != CatCode::L_BRACE) {    // has FuncFParams
            params = parseFuncFParams();
        }
    }
    if (peek.type != CatCode::R_PARENT) {   // must be lack ')' now
        /* throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack )"; */
        ErrorHandler::respond(ErrCode::LACK_R_PARENT, preLook(-1).lno);                     // Error: j
    } else {
        nextWord();
    }

    /* got full function head so we can add it to the current symbol table */
    curContext->addFunc(funcName, ftype, params, f_lno);

    /* parse function block and generate new symbol table */
    parseBlock(ftype != Type::VOID, params);

    irBuilder.addItemDefEnd(funcName);

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
            int GET_number;
            nextWord();
            parseConstExp(GET_number);
            param.dim.push_back(GET_number); // value from parseConstExp()
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
    vector<string> collectRParams;
    string GET_symbol;
    vector<Param> params;

    params.push_back(parseExp(&GET_symbol));
    collectRParams.emplace_back(GET_symbol);

    while (peek.type == CatCode::COMMA) {
        nextWord();
        params.push_back(parseExp(&GET_symbol));
        collectRParams.emplace_back(GET_symbol);
    }

    for (string& symbol: collectRParams) {
        irBuilder.addItemLoadRParam(symbol);
    }
    genOutput("<FuncRParams>");
    return params;
}