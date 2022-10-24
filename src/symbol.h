//
// Created by Excalibur on 2022/10/8.
//

#ifndef COMPILER_SYMBOL_H
#define COMPILER_SYMBOL_H

#include "lexer.h"
#include "errcode.h"
#include <string>
#include <vector>
#include <map>
using namespace std;

extern unsigned int no_cnt;

/*========== type define ==========*/
enum struct Kind {
    VAR,
    FUNC
};

enum struct Type {
    VOID,
    INT
};

struct Para {
    Type type;
    vector<int> dim;
    string name;
};

/*========== item define ==========*/
struct SymbolItem {
    unsigned int const no = no_cnt++;
    string name;
    Kind kind;
    Type type;
    int addr = 0;

    SymbolItem(string& n, Kind k, Type t): name(n), kind(k), type(t) {}
};

struct FuncItem : public SymbolItem {
    vector<Para> paras;

    FuncItem(string& name, Type rtype): SymbolItem(name, Kind::FUNC, rtype) {}
};

struct IdentItem : public SymbolItem {
    vector<int> dim;
    bool modifiable;

    IdentItem(string& name, bool m): SymbolItem(name, Kind::VAR, Type::INT), modifiable(m) {}
};

/*========== table define ==========*/
class SymbolTable {
private:
    map<string, SymbolItem*> items;
    SymbolTable* preContext = nullptr;

public:
    bool hasSymbol(string& symbol) {  // for this SymbolTable
        return items.find(symbol) != items.end();
    }

    bool haveSymbol(string& symbol) { // for all reachable SymbolTables
        if (hasSymbol(symbol)) return true;
        if (preContext != nullptr)
            return preContext->haveSymbol(symbol);
        return false;
    }

    void addIdent(Word& word) {
        auto identItem = new IdentItem(word.cont, false);
        // cout << word.cont << endl;
        if (hasSymbol(word.cont)) {
            ErrorHandler::respond(ErrCode::REDEFINE_IDENT, word.lno); // b
            return ;
        }
        items[word.cont] = identItem;
    }
};

extern SymbolTable* curContext;

#endif //COMPILER_SYMBOL_H
