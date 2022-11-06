//
// Created by Excalibur on 2022/10/8.
//

#ifndef COMPILER_SYMBOL_H
#define COMPILER_SYMBOL_H

#include "lexer.h"
#include "errcode.h"
#include "catcode.h"
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

/* chaos design */
struct Param {  // function parameter -> a parameter exp will return
    Type type;
    vector<int> dim;    // dim[0], if exist, should always be 0
    string name;

    Param():type(Type::INT), name("") {}  // couldn't pass "nullptr" to name
};

/*========== item define ==========*/
struct SymbolItem {
    unsigned int const no = no_cnt++;
    string name;
    Kind kind;
    Type type;
    int addr = 0;

    SymbolItem(string& n, Kind k, Type t): name(n), kind(k), type(t) {}
    virtual ~SymbolItem() = default;
};

struct FuncItem : public SymbolItem {
    vector<Param> params;

    FuncItem(string& name, Type rtype, vector<Param>&& p)
                : SymbolItem(name, Kind::FUNC, rtype), params(p) {}
};

struct IdentItem : public SymbolItem {
    vector<int> dim;
    bool modifiable;

    IdentItem(string& name, bool m)
                : SymbolItem(name, Kind::VAR, Type::INT), modifiable(m){}
    IdentItem(string& name, bool m, vector<int>&& d)
                : SymbolItem(name, Kind::VAR, Type::INT), dim(d), modifiable(m){}
};

/*========== table define ==========*/
class SymbolTable {
private:
    map<string, SymbolItem*> items;
    SymbolTable* preContext = nullptr;
    int layerNo = 0;
    bool needReturn = false;    // deal with "inner return for void"
    bool inLoop = false;        // deal with "continue out of while"

public:
    SymbolTable() = default;
    /* from normal block */
    SymbolTable(SymbolTable* pre, bool new_inLoop) {
        preContext = pre;
        layerNo = pre->layerNo + 1;
        needReturn = pre->needReturn;
        inLoop = pre->isInLoop() || new_inLoop;
    }
    /* from function block -> init */
    SymbolTable(SymbolTable* pre, vector<Param>& preVars, bool needReturnValue) {
        preContext = pre;
        layerNo = pre->layerNo + 1;
        needReturn = needReturnValue;
        inLoop = false;
        for (auto& var: preVars) {
            auto identItem = new IdentItem(var.name, true, std::move(var.dim));
            items[var.name] = identItem;
        }
    }
    ~SymbolTable() {
        for (const auto& item : items)
            delete item.second;
    }

    SymbolTable* getPreContext() {
        return preContext;
    }

    int getLayerNo() {
        return layerNo;
    }

    bool isNeedReturn() {
        return needReturn;
    }

    bool isInLoop() {
        return inLoop;
    }

    bool hasSymbol(string& symbol) {  // for this SymbolTable
        return items.find(symbol) != items.end();
    }

    bool haveSymbol(string& symbol) { // for all reachable SymbolTables
        if (hasSymbol(symbol)) return true;
        if (preContext != nullptr)
            return preContext->haveSymbol(symbol);
        return false;
    }

    bool hasIdent(string& identName) {
        auto iter = items.find(identName);
        if (iter == items.end()) return false;
        return iter->second->kind == Kind::VAR;
    }

    bool haveIdent(string& identName) {
        if (hasIdent(identName)) return true;
        if (preContext != nullptr)
            return preContext->haveIdent(identName);
        return false;
    }

    IdentItem* addIdent(Word& word, bool modifiable) {
        auto identItem = new IdentItem(word.cont, modifiable);
        // cout << word.cont << endl;
        if (hasSymbol(word.cont)) {
            ErrorHandler::respond(ErrCode::REDEFINE_IDENT, word.lno);   // Error: b
            return nullptr;
        }
        items[word.cont] = identItem;
        return identItem;
    }

    IdentItem* getIdent(string& identName) {
        SymbolTable* tmp = this;
        while (tmp != nullptr) {
            auto iter = tmp->items.find(identName);
            if (iter != tmp->items.end() && iter->second->kind == Kind::VAR)
                return dynamic_cast<IdentItem*>(iter->second);
            tmp = tmp->preContext;
        }
        return nullptr;
    }

    bool hasFunc(string& funcName) {
        SymbolTable* tmp = preContext;
        while (tmp->preContext != nullptr)
            tmp = tmp->preContext;

        auto iter = tmp->items.find(funcName);
        if (iter == tmp->items.end()) return false;
        return iter->second->kind == Kind::FUNC;
    }

    FuncItem* addFunc(string& fname, Type ftype, vector<Param> params, int f_lno) {
        auto funcItem = new FuncItem(fname, ftype, std::move(params));
        if (hasSymbol(fname)) {
            ErrorHandler::respond(ErrCode::REDEFINE_IDENT, f_lno);      // Error: b
            return nullptr;
        }
        items[fname] = funcItem;
        return funcItem;
    }

    FuncItem* getFunc(string& funcName) {
        SymbolTable* tmp = preContext;
        while (tmp->preContext != nullptr)
            tmp = tmp->preContext;

        auto iter = tmp->items.find(funcName);
        if (iter == tmp->items.end() || iter->second->kind == Kind::VAR) return nullptr;
        return dynamic_cast<FuncItem*>(iter->second);
    }
};

extern SymbolTable* curContext;

#endif //COMPILER_SYMBOL_H
