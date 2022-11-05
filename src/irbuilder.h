//
// Created by Excalibur on 2022/10/31.
//

#ifndef COMPILER_IR_BUILDER_H
#define COMPILER_IR_BUILDER_H

#include <vector>
#include <string>
#include <map>
#include <bitset>
#include <fstream>
#include <iostream>
#include "catcode.h"

using namespace std;

enum struct IROp {  // | op | label1 | label2 | res |
    DEF_CON,        //   *     int       1       a      <-- const int a = 3;
    DEF_VAR,        //   *     int       3       b      <-- int b[3] = {1,2,3};
    DEF_INIT,       //   *      3               b[2]    <-- (from above)

    DEF_END,        //   *                     ident    <-- mark end of def

    DEF_FUN,        //   *   int/void           foo     <-- int/void foo(int a, int b[], int c[][3])
    FPARA,          //   *     int       2     c[][3]   <-- (from above)
    RET,            //   *    ident                     <-- return ident

    RPARA,          //   *
    CALL_FUN,       //   *

    ADD,
    MIN,
    MUL,
    DIV,
    MOD,

    LABEL,
    CMP,
    BEQ,
    BNE,

    LOAD_ARR,       //   *     arr      3        t1     <-- t1=a[3]
    STORE_ARR,      //   *     arr      3        t1     <-- a[3]=t1

    PRINTF,
    SCANF,

    END,
    P_HOLDER
};

void reverseIROp(IROp& input);
string IROpToString(IROp op);

struct IRItem {
    IROp op;
    string label1;
    string label2;
    string res;

    IRItem(): op(IROp::P_HOLDER), label1(""), label2(""), res("") {}
    IRItem(IROp o, string& l1, string& l2, string& r): op(o), label1(l1), label2(l2), res(r) {}
    IRItem(IROp o, string& l1): op(o), label1(l1), label2(""), res("") {}
    string toString() {
        if (op == IROp::STORE_ARR || op == IROp::LOAD_ARR) {
            return IROpToString(op) + " " + res + " " + label1 + "[" + label2 + "]";
        }
        return IROpToString(op) + " " + res + " " + label1 + " " + label2;
    }
};

#define TMP_SIZE 128

class IRBuilder {
private:
    string ZERO_STR = "0";
    string NULL_STR = "";
    string INT_STR  = "int";

    ofstream ofs;
    bool isprint = true;

    vector<IRItem>& IRs;

    bitset<TMP_SIZE> tmpPools;

    string genTmpSymbol() {
        for (int no = 1; no <= TMP_SIZE; ++no) {
            if (!tmpPools[no]) {
                tmpPools[no] = true;
                return "@t" + to_string(no);
            }
        }
        cout << "no space for tmp variable" << endl;
        return "";
    }

    void freeTmpSymbol(string& tmpSymbol) {
        if (tmpSymbol.substr(0,2) == "@t") {
            int no = stoi(tmpSymbol.substr(2));
            tmpPools[no] = false;
        }
    }

public:
    int inEffect = true;
    const map<CatCode, IROp> catCode2IROp = {
            {CatCode::PLUS  , IROp::ADD },
            {CatCode::MINU  , IROp::MIN },
            {CatCode::MULT  , IROp::MUL },
            {CatCode::DIV   , IROp::DIV },
            {CatCode::MOD   , IROp::MOD },
    };

    explicit IRBuilder(vector<IRItem> &irs);

    void printIRs();

    string addItemCalculateExp(IROp op, string&& label1, string&& label2);
    string addItemCalculateExp(IROp op, string& label1, string& label2);
    string addItemCalculateExp(IROp op, string& label1, string&& label2);

    void addItemAssign(string& lvalue, string& rvalue);

    string addItemLoadArray(string& array, string& offset);
    void addItemStoreArray(string& array, string& offset, string& ident);

    void addItemLoadRParam(string& rParam);
    void addItemCallFunc(string& funcName);
    void addItemFuncReturn(string& retValue);

    void addItemPrintf(string& formatString);
    void addItemPrintf(string&& formatString);

    string addItemScanf();

    void addItemDef(IROp defOp, string&& dimSize, string& ident, bool isArray);
    void addItemDefInit(string& ident, string&& value);

    void addItemDefFunc(string& funcType, string& ident);
    void addItemDefFParam(string& paramName, string&& dimSize);

    void addItemDefEnd(string& ident);
};

extern vector<IRItem> IRList;
extern IRBuilder irBuilder;

#endif //COMPILER_IR_BUILDER_H
