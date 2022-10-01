//
// Created by Excalibur on 2022/9/20.
//

#include "parser.h"
#include "catcode.h"
#include <iostream>

using namespace std;

void doSyntaxAnalysis() {
    if (wordList.empty())
        throw "no word to analysis!";

    Parser parser(wordList);
    try {
        parser.parseCompUnit();
    } catch (string& err_str) {
        cout << err_str << endl;
    }

}


/*========================= class Parser =========================*/
Parser::Parser(vector<Word> &w): wordsList(w), cnt(1), peek(w[0]) {
    ofs.open("output.txt", ios::out);
    if (ofs.fail()) {
        cerr << "failed to write!" << endl;
        return;
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
    ofs << "<CompUnit>" << endl;
}

void Parser::parseMainFunc() {
    if (peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack int";
    if (nextWord().type != CatCode::MAIN_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack main";
    if (nextWord().type != CatCode::L_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack (";
    if (nextWord().type != CatCode::R_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] main: lack )";
    nextWord(); parseBlock();
    ofs << "<MainFuncDef>" << endl;
}

void Parser::parseBlock() {
    if (peek.type != CatCode::L_BRACE)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] block: not a {";
    nextWord();
    while (peek.type != CatCode::R_BRACE) {
        parseBlockItem();
    }
    nextWord();
    ofs << "<Block>" << endl;
}

void Parser::parseBlockItem() {
    switch(peek.type) {
        // Decl
        case CatCode::CONST_TK :
            parseConstDecl();
            break;
        case CatCode::INT_TK:
            parseVarDecl();
            break;

        // Stmt
        default: parseStmt();
    }
}

void Parser::parseStmt() {
    switch (peek.type) {
        // 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
        case CatCode::IF_TK :
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] if: lack (";
            nextWord(); parseCond();
            if (peek.type != CatCode::R_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] if: lack )";
            nextWord(); parseStmt();
            if (peek.type == CatCode::ELSE_TK) {
                nextWord(); parseStmt();
            }
            break;

        // 'while' '(' Cond ')' Stmt
        case CatCode::WHILE_TK :
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] while: lack (";
            nextWord(); parseCond();
            if (peek.type != CatCode::R_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] while: lack )";
            nextWord(); parseStmt();
            break;

        // 'break' ';' | 'continue' ';'
        case CatCode::BREAK_TK :
        case CatCode::CONTINUE_TK :
            if (nextWord().type != CatCode::SEMICN)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] break/continue: lack ;";
            nextWord();
            break;

        // 'return' [Exp] ';'
        case CatCode::RETURN_TK:
            nextWord(); parseExp();
            if (peek.type != CatCode::SEMICN)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] return: lack ;";
            nextWord();
            break;

        // 'printf''('FormatString{','Exp}')'';'
        case CatCode::PRINTF_TK:
            if (nextWord().type != CatCode::L_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack (";
            if (nextWord().type != CatCode::STR_CON)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack format-string";
            nextWord();
            while (peek.type == CatCode::COMMA) {
                nextWord(); parseExp();
            }
            if (peek.type != CatCode::R_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack )";
            if (nextWord().type != CatCode::SEMICN)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] printf: lack ;";
            nextWord();
            break;

        // Block
        case CatCode::L_BRACE:
            parseBlock();
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
            if (nextWord().type != CatCode::SEMICN)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] Exp: lack ;";
            nextWord();
            break;
        // Exp ';' / LVal '=' xxx ';'
        case CatCode::IDENFR:
            if (preLook(1).type == CatCode::L_PARENT) // Exp ';' (Ident() function)
                parseExp();
            else {
                snapshot(); parseLVal();
                if (peek.type == CatCode::ASSIGN) { // LVal '='
                    recover();
                    parseLVal();
                    if (nextWord().type == CatCode::GETINT_TK) {
                        if (nextWord().type != CatCode::L_PARENT)
                            throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal = getint: lack (";
                        if (nextWord().type != CatCode::R_PARENT)
                            throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal = getint: lack )";
                        nextWord();
                    } else parseExp();
                } else {                        // Exp ';'
                    recover();
                    parseExp();
                }
            }
            if (peek.type != CatCode::SEMICN)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] Exp / LVal= : lack ;";
            nextWord();
            break;

        default: throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized Stmt";
    }
    ofs << "<Stmt>" << endl;
}

void Parser::parseCond() {
    parseLOrExp();
    ofs << "<Cond>" << endl;
}

void Parser::parseExp() {
    parseAddExp();
    ofs << "<Exp>" << endl;
}

void Parser::parseConstExp() {
    parseAddExp();
    ofs << "<ConstExp>" << endl;
}

void Parser::parseUnaryExp() { // PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
    switch (peek.type) {
        // UnaryOp UnaryExp
        case CatCode::PLUS:
        case CatCode::MINU:
        case CatCode::NOT :
            parseUnaryOp();
            parseUnaryExp();
            break;

        // Ident
        case CatCode::IDENFR:
            if (preLook(1).type == CatCode::L_PARENT) { // Ident '(' [FuncRParams] ')'
                nextWord();
                if (nextWord().type != CatCode::R_PARENT) {
                    parseFuncRParams();
                    if (peek.type != CatCode::R_PARENT)
                        throw "[" + to_string(peek.lno) + " " + peek.cont + "] function call: lack )";
                }
                nextWord();
            } else {
                parsePrimaryExp();
            }
            break;

        case CatCode::L_PARENT:
        case CatCode::INT_CON :
            parsePrimaryExp();
            break;
        default: throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized UnaryExp";
    }
    ofs << "<UnaryExp>" << endl;
}

void Parser::parseLVal() { // Ident {'[' Exp ']'}
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal: lack Ide";
    nextWord();
    while (peek.type == CatCode::L_BRACK) {
        nextWord(); parseExp();
        if (peek.type != CatCode::R_BRACK)
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] LVal: array lack ]";
        nextWord();
    }
    if (!inrecord) ofs << "<LVal>" << endl;
}

void Parser::parsePrimaryExp() { // '(' Exp ')' | LVal | Number
    switch (peek.type) {
        // '(' Exp ')'
        case CatCode::L_PARENT :
            nextWord(); parseExp();
            if (peek.type != CatCode::R_PARENT)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] PrimaryExp-(Exp): lack )";
            nextWord();
            break;

        // Number
        case CatCode::INT_CON :
            parseNumber();
            break;

        // LVal
        case CatCode::IDENFR :
            parseLVal();
            break;

        default: throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized PrimaryExp";
    }
    ofs << "<PrimaryExp>" << endl;
}

void Parser::parseConstDecl() { // 'const' BType ConstDef { ',' ConstDef } ';'
    if (peek.type != CatCode::CONST_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack const\n"; // unnecesry
    if (nextWord().type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack BType(int)";
    do {
        nextWord(); parseConstDef();
    } while (peek.type == CatCode::COMMA);
    if (peek.type != CatCode::SEMICN)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDecl: lack ;";
    nextWord();
    ofs << "<ConstDecl>" << endl;
}

void Parser::parseConstDef() { // Ident { '[' ConstExp ']' } '=' ConstInitVal
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: lack IDent";
    while (nextWord().type == CatCode::L_BRACK) {
        nextWord(); parseConstExp();
        if (peek.type != CatCode::R_BRACK)
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: array lack ] to match [";
    }
    if (peek.type != CatCode::ASSIGN)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] ConstDef: lack =";
    nextWord(); parseConstInitVal();
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
    ofs << "<ConstInitVal>" << endl;
}

void Parser::parseVarDecl() { // BType VarDef { ',' VarDef } ';'
    if (peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDecl: lack BType(int)";
    do {
        nextWord(); parseVarDef();
    } while (peek.type == CatCode::COMMA);
    if (peek.type != CatCode::SEMICN)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDecl: lack ;";
    nextWord();
    ofs << "<VarDecl>" << endl;
}

void Parser::parseVarDef() { // Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDef: lack Ident";
    while (nextWord().type == CatCode::L_BRACK) {
        nextWord(); parseConstExp();
        if (peek.type != CatCode::R_BRACK)
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] VarDef: array lack ] to match [";
    }
    if (peek.type == CatCode::ASSIGN) { // ... '=' InitVal
        nextWord(); parseInitVal();
    }
    ofs << "<VarDef>" << endl;
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
    ofs << "<InitVal>" << endl;
}

void Parser::parseFuncDef() {
    parseFuncType();
    if (peek.type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack Ident";
    if (nextWord().type != CatCode::L_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack (";
    if (nextWord().type != CatCode::R_PARENT) {
        parseFuncFParams();
    }
    if (peek.type != CatCode::R_PARENT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncDef: lack )";
    nextWord(); parseBlock();
    ofs << "<FuncDef>" << endl;
}

void Parser::parseFuncFParams() {
    parseFuncFParam();
    while (peek.type == CatCode::COMMA) {
        nextWord(); parseFuncFParam();
    }
    ofs << "<FuncFParams>" << endl;
}

void Parser::parseFuncFParam() { // BType Ident ['[' ']' { '[' ConstExp ']' }]
    if (peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncFParam: lack BType(int)";
    if (nextWord().type != CatCode::IDENFR)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncFParam: lack Ident";
    if (nextWord().type == CatCode::L_BRACK) { // '[' ']' { '[' ConstExp ']' }
        if (nextWord().type != CatCode::R_BRACK)
            throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncFParam: array param lack ] to match [";
        while (nextWord().type == CatCode::L_BRACK) { // { '[' ConstExp ']' }
            nextWord(); parseConstExp();
            if (peek.type != CatCode::R_BRACK)
                throw "[" + to_string(peek.lno) + " " + peek.cont + "] FuncFParam: mul arrays param lack ] to match [";
        }
    }
    ofs << "<FuncFParam>" << endl;
}

void Parser::parseFuncRParams() {
    parseExp();
    while (peek.type == CatCode::COMMA) {
        nextWord(); parseExp();
    }
    ofs << "<FuncRParams>" << endl;
}