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