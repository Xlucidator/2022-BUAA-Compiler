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
#include "settings.h"

using namespace std;

enum struct IROp {  // | op | label1 | label2 | res |
    DEF_CON,        //   *     int       1       a      <-- const int a = 3;
    DEF_VAR,        //   *     int       3       b      <-- int b[3] = {1,2,3};
    DEF_INIT,       //   *      3       [2]      b      <-- (from above) b[2] = 3;

    DEF_END,        //   *                     ident    <-- mark end of def

    DEF_FUN,        //   *   int/void           foo     <-- int/void foo(int a, int b[], int c[][3])
    FPARA,          //   *   int[][3]    2       c      <-- (from above)
    RET,            //   *    ident                     <-- return ident

    RPARA,          //   *     rt                       <-- foo(rt, ...)
    CALL_FUN,       //   *     foo                      <-- (from above)
    GET_RET,        //   *     @t1                      <-- foo() => @t1

    SET,            //   *     rs               rd      <-- rd = rs
    ADD,            //   *     rs       rt      rd      <-- rd = rs + rt
    MIN,            //   *     rs       rt      rd      <-- rd = rs - rt
    MUL,            //   *     rs       rt      rd      <-- rd = rs * rt
    DIV,            //   *     rs       rt      rd      <-- rd = rs / rt
    MOD,            //   *     rs       rt      rd      <-- rd = rs % rt
    AND,            //   *     rs       rt      rd      <-- rd = rs && rt
    OR ,            //   *     rs       rt      rd      <-- rd = rs || rt

    LABEL,          //   *                     label    <-- label:
    BEQ,            //   *     rs       rt     label    <-- if rs == rt goto label
    BNE,            //   *     rs       rt     label    <-- if rs != rt goto label
    JUMP,           //   *                     label    <-- goto label

    SEQ,            //   *     rs       rt      rd      <-- rd = (rs == rt) ? 1 : 0
    SNE,            //   *     rs       rt      rd      <-- rd = (rs != rt) ? 1 : 0
    SLT,            //   *     rs       rt      rd      <-- rd = (rs <  rt) ? 1 : 0
    SLE,            //   *     rs       rt      rd      <-- rd = (rs <= rt) ? 1 : 0
    SGT,            //   *     rs       rt      rd      <-- rd = (rs >  rt) ? 1 : 0
    SGE,            //   *     rs       rt      rd      <-- rd = (rs >= rt) ? 1 : 0

    LOAD_ARR,       //   *     arr      3        t1     <-- t1=a[3]
    STORE_ARR,      //   *     arr      3        t1     <-- a[3]=t1

    PRINTF,         //   *    content                   <-- transform to (printf content)
    SCANF,          //   *     @t1                      <-- getint: transform to (scanf @t1)

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
    IRItem(IROp o, string&& l1, string&& l2, string& r): op(o), label1(l1), label2(l2), res(r) {}
    IRItem(IROp o, string&& l1, string& l2, string&& r): op(o), label1(l1), label2(l2), res(r) {}
    IRItem(IROp o, string&& l1, string& l2, string& r): op(o), label1(l1), label2(l2), res(r) {}
    IRItem(IROp o, string& l1, string& l2, string&& r): op(o), label1(l1), label2(l2), res(r) {}

    IRItem(IROp o, string& l1): op(o), label1(l1), label2(""), res("") {}
    IRItem(IROp o, string&& l1): op(o), label1(l1), label2(""), res("") {}
    string toString() {
        if (op == IROp::STORE_ARR || op == IROp::LOAD_ARR) {
            return IROpToString(op) + " " + res + " " + label1 + "[" + label2 + "]";
        }
        return IROpToString(op) + " " + res + " " + label1 + " " + label2;
    }
};

#define TMP_SIZE (128)

class IRBuilder {
private:
    string ZERO_STR = "0";
    string NULL_STR = "";
    string INT_STR  = "int";

    ofstream ofs;
    bool isprint = IRCODE_PRINT;

    vector<IRItem>& IRs;

    bitset<TMP_SIZE> tmpPools;

    string genTmpSymbol() {
        for (int no = 0; no < TMP_SIZE; ++no) {
            if (!tmpPools[no]) {
                tmpPools[no] = true;
                return "@t" + to_string(no);
            }
        }
        cout << "no space for tmp variable" << endl;
        return "";
    }

    void freeTmpSymbol(string& tmpSymbol) {
        if (tmpSymbol.substr(0,2) == "@t") { // is tmpSymbol
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
            {CatCode::EQL   , IROp::SEQ },
            {CatCode::NEQ   , IROp::SNE },
            {CatCode::LSS   , IROp::SLT },
            {CatCode::LEQ   , IROp::SLE },
            {CatCode::GRE   , IROp::SGT },
            {CatCode::GEQ   , IROp::SGE },
    };

    explicit IRBuilder(vector<IRItem> &irs);

    void printIRs();

    string addItemSet(string& label1);
    string addItemSet(string&& label1);
    string addItemCalculateExp(IROp op, string&& label1, string&& label2);
    string addItemCalculateExp(IROp op, string& label1, string& label2);
    string addItemCalculateExp(IROp op, string& label1, string&& label2);
    string addItemCalculateExp(IROp op, string&& label1, string& label2);

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
    void addItemDefInit(string& ident, string&& index, string&& value);

    void addItemDefFunc(string& funcType, string& funcName);
    void addItemDefFParam(string& paramName, string&& dimSize, string& paramType);

    void addItemDefEnd(string& ident);

    string addItemNot(string& label1);
    string addItemSetAfterCompare(IROp op, string& label1, string& label2);
    void addItemBranchAfterCompare(IROp op, string& label1, string&& label2, string&& tarLabelName);
    void addItemBranchAfterCompare(IROp op, string& label1, string&& label2, string& tarLabelName);
    void addItemLabel(string& labelName);
    void addItemJump(string& labelName);
};

extern vector<IRItem> IRList;
extern IRBuilder irBuilder;

#endif //COMPILER_IR_BUILDER_H
