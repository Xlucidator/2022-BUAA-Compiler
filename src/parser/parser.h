//
// Created by Excalibur on 2022/9/20.
//

#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H


#include <vector>
#include <stack>
#include <fstream>
#include <string>

#include "../tool/tools.h"
#include "../lexer/lexer.h"
#include "symbol.h"
#include "errorhandler/errcode.h"
#include "irbuilder/irbuilder.h"
#include "../settings.h"


using namespace std;

void doSyntaxAnalysis();


class Parser {

private:
    vector<Word> wordsList;
    int cnt;
    Word peek;
    ofstream ofs;
    bool isprint = PARSER_PRINT;

    stack<int> records; // enabled snapshot in record
    bool inrecord = false;

    bool inMain = false;

    string MAIN_STR = "main";
    string RET_MAIN = "#main#";
    string INT_STR = "int";
    string NULL_STR = "";
    string ZERO_STR = "0";

    void snapshot() { // take a snapshot, from which each parseXX become virtual
        records.push(cnt);
        inrecord = !records.empty();
        if (inrecord) {
            ErrorHandler::inEffect = false;
            irBuilder.inEffect = false;
        }
    }

    void recover() {  // must be called after snapshot
        if (inrecord) {
            cnt = records.top();
            peek = wordsList[cnt - 1];
            records.pop();
            inrecord = !records.empty();
            if (!inrecord) {
                // only out of record can ErrorHandle came to effect
                ErrorHandler::inEffect = true;
                irBuilder.inEffect = true;
            }
        }
    }

    void genOutput(string&& tar) {
        isprint && !inrecord && ofs << tar << endl;
    }

public:
    explicit Parser(vector<Word>& w);
    ~Parser();
    inline Word nextWord();
    inline Word prevWord();
    inline Word preLook(int offset);

    // basic structure
    void parseCompUnit();
    void parseMainFunc();
    void parseBlock(bool inLoop, string endLabel);                    // for common block
    void parseBlock(bool needReturnValue, vector<Param>& funcParams); // for function block
    ReturnCheck parseBlockItem(string endLabel);            // ReturnCheck
    ReturnCheck parseStmt(bool inLoop, string endLabel);    // ReturnCheck

    // Exp
    void parseLOrExp(string& symbol);
    void parseLAndExp(string& symbol);
    void parseEqExp(string& symbol);
    void parseRelExp(string& symbol);
    inline Param parseAddExp(string& symbol);
    inline Param parseMulExp(string& symbol);
    Param parseUnaryExp(string& symbol);
    Param parsePrimaryExp(string& symbol);
    IdentItem* parseLVal(vector<int>* offsets, string* symbolLVal, bool wrapArray);

    // Decl
    void parseConstDecl();
    void parseConstDef();
    void parseConstInitVal(string ident, vector<int> dims, int index);
    void parseVarDecl();
    void parseVarDef();
    void parseInitVal(string ident, vector<int> dims, int index);

    // Func
    void parseFuncDef();
    Param parseFuncFParam();
    vector<Param> parseFuncFParams();
    vector<Param> parseFuncRParams();

    // Terminal symbol wrappers
    inline void parseNumber(string& number);
    inline void parseUnaryOp(string& unaryOp);
    inline Type parseBType();
    inline Type parseFuncType();
    // symbol wrappers
    Param parseExp(string* symbol);
    void parseCond(string label);
    void parseConstExp(int& number);
};

inline Word Parser::nextWord() {
    genOutput(getTypeStr(peek.type) + " " + peek.cont);
    // print last one, then peek next

    if (cnt >= wordsList.size())
        return Word(CatCode::EOL);
    peek = wordsList[cnt++];
    return peek;
}

inline Word Parser::prevWord() {
    if (cnt <= 1)
        return Word(CatCode::SOL);
    peek = wordsList[cnt-2];
    cnt = cnt - 1;
    return peek;
}

inline Word Parser::preLook(int offset) {
    if (cnt + offset - 1 < wordsList.size())
        return wordsList[cnt + offset - 1];
    else
        return Word(CatCode::EOL);
}


inline Param Parser::parseAddExp(string& OUT_symbol) {
    // MulExp { ('+' | '−') MulExp }
    string GET_symbolBase;
    string GET_symbolOther;
    IROp GET_expOp;

    Param param;
    param = parseMulExp(GET_symbolBase);
    genOutput("<AddExp>");

    while (peek.type == CatCode::PLUS || peek.type == CatCode::MINU) {
        GET_expOp = irBuilder.catCode2IROp.at(peek.type);
        nextWord();
        parseMulExp(GET_symbolOther); // mergeParam is not necessary
        if (isnumber(GET_symbolBase) && isnumber(GET_symbolOther)) {
            // both symbols are number -> calculate at once
            GET_symbolBase = to_string(calculate(GET_symbolBase, GET_expOp, GET_symbolOther));
        } else {
            if (!isnumber(GET_symbolBase) && hasSign(GET_symbolBase)) {
                // case: (-t) + 3, (-t) + t
                removeSign(GET_symbolBase);
                GET_symbolBase = irBuilder.addItemCalculateExp(IROp::MIN, "0", GET_symbolBase);
            }
            if (hasSign(GET_symbolOther)) { /* optimize: a - -t  ==> a + t */
                // symbolOther has '-', merge is to expOp
                reverseIROp(GET_expOp);
                removeSign(GET_symbolOther);
            }
            GET_symbolBase = irBuilder.addItemCalculateExp(GET_expOp, GET_symbolBase, GET_symbolOther);
        }
        genOutput("<AddExp>");
    }
    OUT_symbol = GET_symbolBase;
    return param;
}

inline Param Parser::parseMulExp(string& OUT_symbol) {
    // UnaryExp { ('*' | '/' | '%') UnaryExp }
    string GET_symbolBase;
    string GET_symbolOther;
    IROp GET_expOp;

    Param param;
    param = parseUnaryExp(GET_symbolBase);
    genOutput("<MulExp>");

    while (
            peek.type == CatCode::MULT ||
            peek.type == CatCode::DIV  ||
            peek.type == CatCode::MOD
            ) {
        GET_expOp = irBuilder.catCode2IROp.at(peek.type);
        nextWord();
        parseUnaryExp(GET_symbolOther); // mergeParam is not necessary
        if (isnumber(GET_symbolBase) && isnumber(GET_symbolOther)) {
            /* case: 3 * -4 ==> -12 */
            GET_symbolBase = to_string(calculate(GET_symbolBase, GET_expOp, GET_symbolOther));
        } else {
            if (hasSign(GET_symbolBase) && hasSign(GET_symbolOther)) {
                /* case: (-t)*(-t), (-t) * -3 ==> t*t, t * 3 */
                removeSign(GET_symbolBase);
                removeSign(GET_symbolOther);
            } else if (!isnumber(GET_symbolBase) && hasSign(GET_symbolBase)) {
                /* case: (-t) * 3 , (-t) * t */
                removeSign(GET_symbolBase);
                GET_symbolBase = irBuilder.addItemCalculateExp(IROp::MIN, "0", GET_symbolBase);
            } else if (!isnumber(GET_symbolOther) && hasSign(GET_symbolOther)) {
                /* case: 3 * (-t) , t * (-t) */
                removeSign(GET_symbolOther);
                GET_symbolOther = irBuilder.addItemCalculateExp(IROp::MIN, "0", GET_symbolOther);
            }
            /* number with sign is ok */
            // finally, we need to mul/div
            GET_symbolBase = irBuilder.addItemCalculateExp(GET_expOp, GET_symbolBase, GET_symbolOther);
        }
        genOutput("<MulExp>");
    }
    OUT_symbol = GET_symbolBase;
    return param;
}

inline void Parser::parseUnaryOp(string& OUT_unaryOp) {
    if (peek.type != CatCode::PLUS && peek.type != CatCode::MINU && peek.type != CatCode::NOT)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized UnaryOp";
    OUT_unaryOp = peek.cont;
    nextWord();
    genOutput("<UnaryOp>");
}

inline void Parser::parseNumber(string& OUT_number) {
    if (peek.type != CatCode::INT_CON)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized Number";
    OUT_number = peek.cont;
    nextWord();
    genOutput("<Number>");
}

inline Type Parser::parseBType() {
    Type type;
    if (peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized BType";
    type = Type::INT;
    nextWord();
    return type;
}

inline Type Parser::parseFuncType() {
    Type rtype;
    if (peek.type != CatCode::VOID_TK && peek.type != CatCode::INT_TK)
        throw "[" + to_string(peek.lno) + " " + peek.cont + "] unrecognized FuncType";
    rtype = (peek.type == CatCode::VOID_TK) ? Type::VOID : Type::INT;
    nextWord();
    genOutput("<FuncType>");
    return rtype;
}

#endif //COMPILER_PARSER_H
