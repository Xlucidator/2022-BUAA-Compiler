//
// Created by Excalibur on 2022/9/20.
//

#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H


#include <vector>
#include <fstream>
#include <string>
#include "lexer.h"
#include "symbol.h"
#include "errcode.h"
using namespace std;

void doSyntaxAnalysis();


class Parser {

private:
    vector<Word> wordsList;
    int cnt;
    int record = -1;
    bool inrecord = false;
    Word peek;
    ofstream ofs;

    bool isprint = true;

    void snapshot() { // take a snapshot, from which each parseXX become virtual
        record = cnt;
        inrecord = true;
        ErrorHandler::inEffect = false;
    }

    void recover() {  // must be called after snapshot, or nothing will be print out
        cnt = record;
        peek = wordsList[record - 1];
        inrecord = false;
        ErrorHandler::inEffect = true;
    }

    void genOutput(string&& tar) { // TODO: implement it in all parsers
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
    void parseBlock(bool inLoop);                           // for common block
    void parseBlock(bool needReturnValue, vector<Param>& funcParams); // for function block
    ReturnCheck parseBlockItem();                           // ReturnCheck
    ReturnCheck parseStmt(bool inLoop);             // ReturnCheck

    // Exp
    inline void parseLOrExp();
    inline void parseLAndExp();
    inline void parseEqExp();
    inline void parseRelExp();
    inline Param parseAddExp();
    inline Param parseMulExp();
    Param parseUnaryExp();
    Param parsePrimaryExp();
    IdentItem* parseLVal(vector<int>* offsets);

    // Decl
    void parseConstDecl();
    void parseConstDef();
    void parseConstInitVal();
    void parseVarDecl();
    void parseVarDef();
    void parseInitVal();

    // Func
    void parseFuncDef();
    Param parseFuncFParam();
    vector<Param> parseFuncFParams();
    vector<Param> parseFuncRParams();

    // Terminal symbol wrappers
    inline void parseNumber();
    inline void parseUnaryOp();
    inline Type parseBType();
    inline Type parseFuncType();
    // symbol wrappers
    Param parseExp();
    void parseCond();
    void parseConstExp();
};

inline Word Parser::nextWord() {
    !isprint && !inrecord &&  ofs << getTypeStr(peek.type) << " " << peek.cont << endl;
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

inline void Parser::parseLOrExp() { // LAndExp { '||' LAndExp }
    parseLAndExp();
    isprint && !inrecord && ofs << "<LOrExp>" << endl;
    if (peek.type != CatCode::OR)
        return;
    while (peek.type == CatCode::OR) {
        nextWord(); parseLAndExp();
        isprint && !inrecord && ofs << "<LOrExp>" << endl;
    }
}

inline void Parser::parseLAndExp() { // EqExp { '&&' EqExp }
    parseEqExp();
    isprint && !inrecord && ofs << "<LAndExp>" << endl;
    if (peek.type != CatCode::AND)
        return;
    while (peek.type == CatCode::AND) {
        nextWord(); parseEqExp();
        isprint && !inrecord && ofs << "<LAndExp>" << endl;
    }
}

inline void Parser::parseEqExp() { // RelExp { ('==' | '!=') RelExp }
    parseRelExp();
    isprint && !inrecord && ofs << "<EqExp>" << endl;
    if (peek.type != CatCode::EQL && peek.type != CatCode::NEQ)
        return;
    while (peek.type == CatCode::EQL || peek.type == CatCode::NEQ) {
        nextWord(); parseRelExp();
        isprint && !inrecord && ofs << "<EqExp>" << endl;
    }
}

inline void Parser::parseRelExp() { // AddExp { ('<' | '>' | '<=' | '>=') AddExp }
    parseAddExp();
    isprint && !inrecord && ofs << "<RelExp>" << endl;
    if (peek.type != CatCode::LSS &&
        peek.type != CatCode::GRE &&
        peek.type != CatCode::LEQ &&
        peek.type != CatCode::GEQ ) return;
    while (
            peek.type == CatCode::LSS ||
            peek.type == CatCode::GRE ||
            peek.type == CatCode::LEQ ||
            peek.type == CatCode::GEQ
            ) {
        nextWord(); parseAddExp();
        isprint && !inrecord && ofs << "<RelExp>" << endl;
    }
}

inline Param Parser::parseAddExp() { // MulExp { ('+' | '−') MulExp }
    Param param;
    param = parseMulExp();
    isprint && !inrecord && ofs << "<AddExp>" << endl;
    if (peek.type != CatCode::PLUS && peek.type != CatCode::MINU)
        return param;
    while (peek.type == CatCode::PLUS || peek.type == CatCode::MINU) {
        nextWord(); parseMulExp();
        // let us assume that in exp all param-types are the same
        isprint && !inrecord && ofs << "<AddExp>" << endl;
    }
    return param;
}

inline Param Parser::parseMulExp() { // UnaryExp { ('*' | '/' | '%') UnaryExp }
    Param param;
    param = parseUnaryExp();
    isprint && !inrecord && ofs << "<MulExp>" << endl;
    if (peek.type != CatCode::MULT &&
        peek.type != CatCode::DIV  &&
        peek.type != CatCode::MOD ) return param;
    while (
            peek.type == CatCode::MULT ||
            peek.type == CatCode::DIV  ||
            peek.type == CatCode::MOD
            ) {
        nextWord(); parseUnaryExp();
        isprint && !inrecord && ofs << "<MulExp>" << endl;
    }
    return param;
}

inline void Parser::parseUnaryOp() {
    if (peek.type != CatCode::PLUS && peek.type != CatCode::MINU && peek.type != CatCode::NOT)
        throw "unrecognized UnaryOp\n";
    nextWord();
    isprint && !inrecord && ofs << "<UnaryOp>" << endl;
}

inline void Parser::parseNumber() {
    if (peek.type != CatCode::INT_CON)
        throw "unrecognized Number\n";
    nextWord();
    isprint && !inrecord && ofs << "<Number>" << endl;
}

inline Type Parser::parseBType() {
    Type type;
    if (peek.type != CatCode::INT_TK)
        throw "unrecognized BType\n";
    type = Type::INT;
    nextWord();
    return type;
}

inline Type Parser::parseFuncType() {
    Type rtype;
    if (peek.type != CatCode::VOID_TK && peek.type != CatCode::INT_TK)
        throw "unrecognized FuncType\n";
    rtype = (peek.type == CatCode::VOID_TK) ? Type::VOID : Type::INT;
    nextWord();
    isprint && !inrecord && ofs << "<FuncType>" << endl;
    return rtype;
}

#endif //COMPILER_PARSER_H
