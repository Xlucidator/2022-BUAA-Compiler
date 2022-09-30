//
// Created by Excalibur on 2022/9/20.
//

#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H


#include <vector>
#include <fstream>
#include "lexer.h"
using namespace std;

void doGrammarAnalysis();


class Parser {

private:
    vector<Word> wordsList;
    int cnt;
    int record = -1;
    bool inrecord = false;
    Word peek;
    ofstream ofs;

    void snapshot() { // take a snapshot, from which each parseXX become virtual
        record = cnt;
        inrecord = true;
    }
    void recover() {  // must be called after snapshot, or nothing will be print out
        cnt = record;
        peek = wordsList[record - 1];
        inrecord = false;
    }

public:
    explicit Parser(vector<Word>& w);
    ~Parser();
    inline Word nextWord();
    inline Word preLook(int offset);

    // basic structure
    void parseCompUnit();
    void parseMainFunc();
    void parseBlock();
    void parseBlockItem();
    void parseStmt();

    // Exp
    inline void parseLOrExp();
    inline void parseLAndExp();
    inline void parseEqExp();
    inline void parseRelExp();
    inline void parseAddExp();
    inline void parseMulExp();
    void parseUnaryExp();
    void parsePrimaryExp();
    void parseLVal();

    // Decl
    void parseConstDecl();
    void parseConstDef();
    void parseConstInitVal();
    void parseVarDecl();
    void parseVarDef();
    void parseInitVal();

    // Func
    void parseFuncDef();
    void parseFuncFParam();
    void parseFuncFParams();
    void parseFuncRParams();

    // Terminal symbol wrappers
    inline void parseNumber();
    inline void parseUnaryOp();
    inline void parseFuncType();
    // symbol wrappers
    void parseExp();
    void parseCond();
    void parseConstExp();
};

inline Word Parser::nextWord() {
    if (!inrecord)
        ofs << getTypeStr(peek.type) << " " << peek.cont << endl;
    // print last one, then peek next

    if (cnt >= wordsList.size())
        return Word(CatCode::EOL);
    peek = wordsList[cnt++];
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
    while (peek.type == CatCode::OR) {
        nextWord(); parseLAndExp();
    }
    ofs << "<LOrExp>" << endl;
}

inline void Parser::parseLAndExp() { // EqExp { '&&' EqExp }
    parseEqExp();
    while (peek.type == CatCode::AND) {
        nextWord(); parseEqExp();
    }
    ofs << "<LAndExp>" << endl;
}

inline void Parser::parseEqExp() { // RelExp { ('==' | '!=') RelExp }
    parseRelExp();
    while (peek.type == CatCode::EQL || peek.type == CatCode::NEQ) {
        nextWord(); parseRelExp();
    }
    ofs << "<EqExp>" << endl;
}

inline void Parser::parseRelExp() { // AddExp { ('<' | '>' | '<=' | '>=') AddExp }
    parseAddExp();
    while (
            peek.type == CatCode::LSS ||
            peek.type == CatCode::GRE ||
            peek.type == CatCode::LEQ ||
            peek.type == CatCode::GEQ
            ) {
        nextWord(); parseAddExp();
    }
    ofs << "<RelExp>" << endl;
}

inline void Parser::parseAddExp() { // MulExp { ('+' | '−') MulExp }
    parseMulExp();
    while (peek.type == CatCode::PLUS || peek.type == CatCode::MINU) {
        nextWord(); parseMulExp();
    }
    ofs << "<AddExp>" << endl;
}

inline void Parser::parseMulExp() { // UnaryExp { ('*' | '/' | '%') UnaryExp }
    parseUnaryExp();
    while (
            peek.type == CatCode::MULT ||
            peek.type == CatCode::DIV  ||
            peek.type == CatCode::MOD
            ) {
        nextWord(); parseUnaryExp();
    }
    ofs << "<MulExp>" << endl;
}

inline void Parser::parseUnaryOp() {
    if (peek.type != CatCode::PLUS && peek.type != CatCode::MINU && peek.type != CatCode::NOT)
        throw "unrecognized UnaryOp\n";
    nextWord();
    ofs << "<UnaryOp>" << endl;
}

inline void Parser::parseNumber() {
    if (peek.type != CatCode::INT_CON)
        throw "unrecognized Number\n";
    nextWord();
    ofs << "<Number>" << endl;
}

inline void Parser::parseFuncType() {
    if (peek.type != CatCode::VOID_TK && peek.type != CatCode::INT_TK)
        throw "unrecognized FuncType\n";
    nextWord();
    ofs << "<FuncType>" << endl;
}

#endif //COMPILER_PARSER_H
