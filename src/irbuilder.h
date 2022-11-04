//
// Created by Excalibur on 2022/10/31.
//

#ifndef COMPILER_IR_BUILDER_H
#define COMPILER_IR_BUILDER_H

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include "catcode.h"

using namespace std;

enum struct IROp {  // | op | label1 | label2 | res |
    DEF_CON,        //   *     int       0       a      <-- const int a = 3;
    DEF_VAR,        //   *     int       1       b      <-- int b[3] = {1,2,3};
    DEF_INIT,       //   *      3               b[2]    <-- (from above)

    DEF_END,        //   *                     ident    <-- mark end of def

    DEF_FUN,        //   *   int/void           foo     <-- int/void foo(int a, int b[], int c[][3])
    FPARA,          //   *     int       2     c[][3]   <-- (from above)
    RET,            //   *    ident                     <-- return ident

    RPARA,
    CALL_FUN,

    ADD,
    MIN,
    MUL,
    DIV,
    MOD,

    LABEL,
    CMP,
    BEQ,
    BNE,

    LOAD_ARR,       // [ ] a 3 t1  <--  t1=a[3]
    STORE_ARR,      // [*] t1 3 a  <--  a[3]=t1

    PRINTF,
    SCANF
};

void reverseIROp(IROp& input);
string IROpToString(IROp op);

struct IRItem {
    IROp op;
    string label1;
    string label2;
    string res;

    IRItem(IROp o, string& l1, string& l2, string& r): op(o), label1(l1), label2(l2), res(r) {}
    IRItem(IROp o, string& l1): op(o), label1(l1), label2(""), res("") {}
    string toString() {
        return IROpToString(op) + " " + res + " " + label1 + " " + label2;
    }
};

class IRBuilder {
private:
    int no = 1;
    string ZERO_STR = "0";
    string NULL_STR = "";
    string INT_STR  = "int";

    ofstream ofs;
    bool isprint = true;

    vector<IRItem> IRs;

    string genTmpSymbol() {
        return "@t" + to_string(no++);
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

    IRBuilder() = default;
    explicit IRBuilder(vector<IRItem> &irs): IRs(irs) {
        if (isprint) {
            ofs.open("irs.txt", ios::out);
            if (ofs.fail()) {
                 cerr << "failed to write" << endl;
                 return;
            }
        }
    }

    void printIRs() {
        if (isprint) {
            string prefix;
            bool STATE_FUNC = false;
            string funcName;
            for (auto &item : IRs) {
                if (STATE_FUNC && item.op == IROp::DEF_END && item.res == funcName) {
                    prefix = prefix.substr(0, prefix.length()-1);
                    STATE_FUNC = false;
                }

                ofs << prefix << item.toString() << endl;

                if (item.op == IROp::DEF_FUN) {
                    STATE_FUNC = true;
                    funcName = item.res;
                    prefix += "\t";
                }

                if (item.toString().find("END") != string::npos) {
                    ofs << prefix << endl;
                }
            }
        }
    }

    string addItemCalculateExp(IROp op, string&& label1, string&& label2);
    string addItemCalculateExp(IROp op, string& label1, string& label2);
    string addItemCalculateExp(IROp op, string& label1, string&& label2);

    void addItemAssign(string& lvalue, string& rvalue) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::ADD, rvalue, ZERO_STR, lvalue));
        }
    }

    void addItemLoadRParam(string& rParam) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::RPARA, rParam));
        }
    }

    void addItemCallFunc(string& funcName) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::CALL_FUN, funcName));
        }
    }

    void addItemFuncReturn(string& retValue) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::RET, retValue));
        }
    }

    void addItemPrintf(string& formatString) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::PRINTF, formatString));
        }
    }

    string addItemScanf() {
        if (inEffect) {
            string tmpSymbol = genTmpSymbol();
            IRs.emplace_back(IRItem(IROp::SCANF, tmpSymbol));
            return tmpSymbol;
        }
        return "";
    }

    void addItemDef(IROp defOp, string&& dimSize, string& ident) {
        if (inEffect) {
            IRs.emplace_back(IRItem(defOp, INT_STR, dimSize, ident));
        }
    }

    void addItemDefInit(string& ident, string&& value) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::DEF_INIT, value, NULL_STR, ident));
        }
    }

    void addItemDefFunc(string& funcType, string& ident) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::DEF_FUN, funcType, NULL_STR, ident));
        }
    }

    void addItemDefFParam(string& paramName, string&& dimSize) {
        // ident will carry dim info
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::FPARA, INT_STR, dimSize, paramName));
        }
    }

    void addItemDefEnd(string& ident) {
        if (inEffect) {
            IRs.emplace_back(IRItem(IROp::DEF_END, NULL_STR, NULL_STR, ident));
        }
    }
};

extern vector<IRItem> IRList;
extern IRBuilder irBuilder;

#endif //COMPILER_IR_BUILDER_H
