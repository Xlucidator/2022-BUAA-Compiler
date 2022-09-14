//
// Created by Excalibur on 2022/9/12.
//
#include "lexer.h"
#include "catcode.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

vector<Word> wordList;


void doLexicalAnalysis(ifstream& ifs) {
    string code_buf;
    Lexer lexer;
    while (getline(ifs, code_buf)) { // '\n' will not be stored in
        if (code_buf.empty()) continue;
        lexer.refillStr(code_buf);
        lexer.lexing();
    }
    fprintWordList();
}


void printWordList() {
    for (Word& word : wordList) {
        cout << getTypeStr(word.type)  << " " << word.cont << endl;
    }
}

void fprintWordList() {
    ofstream ofs;
    ofs.open("output.txt", ios::out);
    if (ofs.fail()) {
        cerr << "failed to write!" << endl;
        return;
    }

    for (Word& word : wordList) {
        ofs << getTypeStr(word.type)  << " " << word.cont << endl;
    }

    ofs.close();
}


string getTypeStr(CatCode type) {
    switch (type) {
        case CatCode::IDENFR      : return "IDENFR"    ;
        case CatCode::INT_CON     : return "INTCON"    ;
        case CatCode::STR_CON     : return "STRCON"    ;
        case CatCode::MAIN_TK     : return "MAINTK"    ;
        case CatCode::CONST_TK    : return "CONSTTK"   ;
        case CatCode::INT_TK      : return "INTTK"     ;
        case CatCode::BREAK_TK    : return "BREAKTK"   ;
        case CatCode::CONTINUE_TK : return "CONTINUETK";
        case CatCode::IF_TK       : return "IFTK"      ;
        case CatCode::ELSE_TK     : return "ELSETK"    ;
        case CatCode::NOT         : return "NOT"       ;
        case CatCode::AND         : return "AND"       ;
        case CatCode::OR          : return "OR"        ;
        case CatCode::WHILE_TK    : return "WHILETK"   ;
        case CatCode::GETINT_TK   : return "GETINTTK"  ;
        case CatCode::PRINTF_TK   : return "PRINTFTK"  ;
        case CatCode::RETURN_TK   : return "RETURNTK"  ;
        case CatCode::PLUS        : return "PLUS"      ;
        case CatCode::MINU        : return "MINU"      ;
        case CatCode::VOID_TK     : return "VOIDTK"    ;
        case CatCode::MULT        : return "MULT"      ;
        case CatCode::DIV         : return "DIV"       ;
        case CatCode::MOD         : return "MOD"       ;
        case CatCode::LSS         : return "LSS"       ;
        case CatCode::LEQ         : return "LEQ"       ;
        case CatCode::GRE         : return "GRE"       ;
        case CatCode::GEQ         : return "GEQ"       ;
        case CatCode::EQL         : return "EQL"       ;
        case CatCode::NEQ         : return "NEQ"       ;
        case CatCode::ASSIGN      : return "ASSIGN"    ;
        case CatCode::SEMICN      : return "SEMICN"    ;
        case CatCode::COMMA       : return "COMMA"     ;
        case CatCode::L_PARENT    : return "LPARENT"   ;
        case CatCode::R_PARENT    : return "RPARENT"   ;
        case CatCode::L_BRACK     : return "LBRACK"    ;
        case CatCode::R_BRACK     : return "RBRACK"    ;
        case CatCode::L_BRACE     : return "LBRACE"    ;
        case CatCode::R_BRACE     : return "RBRACE"    ;
        default: return "";
    }
}

/*========================= class Lexer =========================*/
Lexer::Lexer() {
    pos = 0;
    state = S_INIT;
    eol = true;
}

void Lexer::refillStr(string& input) {
    str = input;
    pos = 0;
    eol = false;
}

void Lexer::lexing() {
    string gettoken;
    while (!eol) {
        Word word;
        gettoken = nextToken();
        // cout << "get something:" << gettoken << endl;
        if (!gettoken.empty()) {
            word.cont = gettoken;
            word.type = getWordType(gettoken);
            wordList.push_back(word);
        }
    }
}


string Lexer::nextToken() {
    string token;
    char c;

    while (state != S_END && pos < str.length()) {
        c = str[pos++];
        switch (state) {
            case S_INIT:
                //token = "";
                if (isalpha(c) || c == '_') {
                    token += c;
                    state = S_IDENT;
                }
                else if (isdigit(c)) {
                    token += c;
                    state = S_NUM;
                }
                else if (c == '\"') {
                    token += c;
                    state = S_STR;
                }
                else if (c == '/' && str[pos] == '*') {
                    pos += 1;
                    state = S_MANN;
                } else if (c == '/' && str[pos] == '/') {
                    pos = (int) str.length();
                }
                else if (dsg.find(c) != string::npos) {
                    token += c;
                    state = S_END;
                }
                else if (ddb.find(c) != string::npos) {
                    token += c;
                    state = S_DDB;
                }
                break;

            case S_MANN:
                if (c == '*' && pos < str.length() && str[pos] == '/') {
                    pos += 1;
                    state = S_INIT;
                }
                break;

            case S_IDENT:
                if (isalpha(c) || isdigit(c) || c == '_')
                    token += c;
                else {
                    pos -= 1;
                    state = S_END;
                }

                break;

            case S_NUM:
                if (isdigit(c))
                    token += c;
                else {
                    pos -= 1;
                    state = S_END;
                }
                break;

            case S_STR:
                token += c;
                if (c == '\"') state = S_END;
                break;

            case S_DDB:
                if (token[0] == '&' || token[0] == '|') {
                    if (c == token[0]) {
                        token += c;
                        state = S_END;
                    } else {
                        state = S_END;// Error
                    }
                }
                else if (c == '=') {
                    token += c;
                    state = S_END;
                }
                else {
                    pos -= 1;
                    state = S_END;
                }
                break;

            default:
                break;
        }
    }
    if (state == S_END) state = S_INIT;
    if (pos >= str.length()) eol = true;

    return token;
}

CatCode Lexer::getWordType(string& token) {
    if (token2Catcode.find(token) != token2Catcode.end()) {
        return token2Catcode.at(token);
    }
    if (token[0] == '\"')  return CatCode::STR_CON;
    if (isdigit(token[0])) return CatCode::INT_CON;
    return CatCode::IDENFR;
}