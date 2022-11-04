//
// Created by Excalibur on 2022/10/31.
//

#include "irbuilder.h"

#include <vector>
#include <string>

using namespace std;

vector<IRItem> IRList;
IRBuilder irBuilder(IRList);  // NOLINT

void reverseIROp(IROp& input) {
    if (input == IROp::ADD)
        input = IROp::MIN;
    if (input == IROp::MIN)
        input = IROp::ADD;
}

string IROpToString(IROp op) {
    switch (op) {
        case IROp::DEF_CON  : return "DEF_CON"  ;
        case IROp::DEF_VAR  : return "DEF_VAR"  ;
        case IROp::DEF_INIT : return "DEF_INIT" ;
        case IROp::DEF_END  : return "DEF_END"  ;
        case IROp::DEF_FUN  : return "DEF_FUN"  ;
        case IROp::FPARA    : return "FPARA"    ;
        case IROp::RET      : return "RET"      ;
        case IROp::RPARA    : return "RPARA"    ;
        case IROp::CALL_FUN : return "CALL_FUN" ;
        case IROp::ADD      : return "ADD"      ;
        case IROp::MIN      : return "MIN"      ;
        case IROp::MUL      : return "MUL"      ;
        case IROp::DIV      : return "DIV"      ;
        case IROp::MOD      : return "MOD"      ;
        case IROp::LABEL    : return "LABEL"    ;
        case IROp::CMP      : return "CMP"      ;
        case IROp::BEQ      : return "BEQ"      ;
        case IROp::BNE      : return "BNE"      ;
        case IROp::LOAD_ARR : return "LOAD_ARR" ;
        case IROp::STORE_ARR: return "STORE_ARR";
        case IROp::PRINTF   : return "PRINTF"   ;
        case IROp::SCANF    : return "SCANF"    ;
        default: return "";
    }
}

void IRBuilder::printIRs() {
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

/*========================= class IRBuilder =========================*/
IRBuilder::IRBuilder(vector<IRItem> &irs): IRs(irs) {
    if (isprint) {
        ofs.open("irs.txt", ios::out);
        if (ofs.fail()) {
            cerr << "failed to write" << endl;
            return;
        }
    }
}

string IRBuilder::addItemCalculateExp(IROp op, string&& label1, string&& label2) {
    if (inEffect) {
        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, label1, label2, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemCalculateExp(IROp op, string& label1, string& label2) {
    if (inEffect) {
        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, label1, label2, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemCalculateExp(IROp op, string& label1, string&& label2) {
    if (inEffect) {
        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, label1, label2, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

void IRBuilder::addItemAssign(string& lvalue, string& rvalue) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::ADD, rvalue, ZERO_STR, lvalue));
    }
}

void IRBuilder::addItemLoadRParam(string& rParam) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::RPARA, rParam));
    }
}

void IRBuilder::addItemCallFunc(string& funcName) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::CALL_FUN, funcName));
    }
}

void IRBuilder::addItemFuncReturn(string& retValue) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::RET, retValue));
    }
}

void IRBuilder::addItemPrintf(string& formatString) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::PRINTF, formatString));
    }
}

void IRBuilder::addItemPrintf(string&& formatString) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::PRINTF, formatString));
    }
}

string IRBuilder::addItemScanf() {
    if (inEffect) {
        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(IROp::SCANF, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

void IRBuilder::addItemDef(IROp defOp, string&& dimSize, string& ident) {
    if (inEffect) {
        IRs.emplace_back(IRItem(defOp, INT_STR, dimSize, ident));
    }
}

void IRBuilder::addItemDefInit(string& ident, string&& value) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::DEF_INIT, value, NULL_STR, ident));
    }
}

void IRBuilder::addItemDefFunc(string& funcType, string& ident) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::DEF_FUN, funcType, NULL_STR, ident));
    }
}

void IRBuilder::addItemDefFParam(string& paramName, string&& dimSize) {
    // ident will carry dim info
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::FPARA, INT_STR, dimSize, paramName));
    }
}

void IRBuilder::addItemDefEnd(string& ident) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::DEF_END, NULL_STR, NULL_STR, ident));
    }
}