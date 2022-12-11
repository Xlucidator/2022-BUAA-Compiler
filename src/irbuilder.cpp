//
// Created by Excalibur on 2022/10/31.
//

#include "irbuilder.h"
#include "tools.h"

#include <vector>
#include <string>

using namespace std;

vector<IRItem> IRList;
IRBuilder irBuilder(IRList);  // NOLINT

void reverseIROp(IROp& input) {
    if (input == IROp::ADD)
        input = IROp::MIN;
    else if (input == IROp::MIN)
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

        case IROp::SET      : return "SET"      ;
        case IROp::ADD      : return "ADD"      ;
        case IROp::MIN      : return "MIN"      ;
        case IROp::MUL      : return "MUL"      ;
        case IROp::DIV      : return "DIV"      ;
        case IROp::MOD      : return "MOD"      ;
        case IROp::AND      : return "AND"      ;
        case IROp::OR       : return "OR"       ;

        case IROp::LABEL    : return "LABEL"    ;
        case IROp::BEQ      : return "BEQ"      ;
        case IROp::BNE      : return "BNE"      ;
        case IROp::JUMP     : return "JUMP"     ;

        case IROp::SEQ      : return "SEQ"      ;
        case IROp::SNE      : return "SNE"      ;
        case IROp::SLT      : return "SLT"      ;
        case IROp::SLE      : return "SLE"      ;
        case IROp::SGT      : return "SGT"      ;
        case IROp::SGE      : return "SGE"      ;

        case IROp::LOAD_ARR : return "LOAD_ARR" ;
        case IROp::STORE_ARR: return "STORE_ARR";

        case IROp::PRINTF   : return "PRINTF"   ;
        case IROp::SCANF    : return "SCANF"    ;

        case IROp::END      : return "END"      ;
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

            if (item.op == IROp::LABEL) {
                ofs << endl;
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

string IRBuilder::addItemSet(string&& label1) {
    if (inEffect) {
        freeTmpSymbol(label1);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(IROp::SET, markUniqueIdent(label1), NULL_STR, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemSet(string& label1) {
    if (inEffect) {
        freeTmpSymbol(label1);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(IROp::SET, markUniqueIdent(label1), NULL_STR, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemCalculateExp(IROp op, string&& label1, string&& label2) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemCalculateExp(IROp op, string& label1, string& label2) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemCalculateExp(IROp op, string& label1, string&& label2) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemCalculateExp(IROp op, string&& label1, string& label2) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

void IRBuilder::addItemAssign(string& lvalue, string& rvalue) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::ADD, markUniqueIdent(rvalue), ZERO_STR, markUniqueIdent(lvalue)));
        freeTmpSymbol(rvalue);
    }
}

string IRBuilder::addItemLoadArray(string &array, string &offset) {
    if (inEffect) {
        freeTmpSymbol(offset);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(IROp::LOAD_ARR, markUniqueIdent(array), offset, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

void IRBuilder::addItemStoreArray(string &array, string &offset, string &ident) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::STORE_ARR, markUniqueIdent(array), offset, markUniqueIdent(ident)));
        freeTmpSymbol(offset);
        freeTmpSymbol(ident);
    }
}

void IRBuilder::addItemLoadRParam(string& rParam) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::RPARA, markUniqueIdent(rParam)));
        freeTmpSymbol(rParam);
    }
}

void IRBuilder::addItemCallFunc(string& funcName) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::CALL_FUN, funcName));
    }
}

void IRBuilder::addItemFuncReturn(string& retValue) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::RET, markUniqueIdent(retValue)));
        freeTmpSymbol(retValue);
    }
}

void IRBuilder::addItemPrintf(string& formatString) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::PRINTF, markUniqueIdent(formatString)));
        freeTmpSymbol(formatString);
    }
}

void IRBuilder::addItemPrintf(string&& formatString) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::PRINTF, markUniqueIdent(formatString)));
        freeTmpSymbol(formatString);
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

void IRBuilder::addItemDef(IROp defOp, string&& indexSize, string& identName, bool isArray) {
    // for DEF_CON | DEF_VAR
    if (inEffect) {
        string type = (isArray) ? "array" : "int" ;
        IRs.emplace_back(IRItem(defOp, type, indexSize, markUniqueIdent(identName)));
    }
}

void IRBuilder::addItemDefInit(string& ident, string&& index, string&& value) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::DEF_INIT, markUniqueIdent(value), index, markUniqueIdent(ident)));
    }
}

void IRBuilder::addItemDefFunc(string& funcType, string& funcName) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::DEF_FUN, funcType, NULL_STR, funcName));
    }
}

void IRBuilder::addItemDefFParam(string& paramName, string&& dimSize, string& paramType) {
    // ident will carry dim info
    if (inEffect) {
        // only after defFParam will the ident be stored to symbol!
        IRs.emplace_back(IRItem(IROp::FPARA, paramType, dimSize, markUniqueIdent(paramName)));
    }
}

void IRBuilder::addItemDefEnd(string& ident) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::DEF_END, NULL_STR, NULL_STR, ident));
    }
}

string IRBuilder::addItemNot(string& label1) {
    if (inEffect) {
        freeTmpSymbol(label1);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(IROp::SEQ, markUniqueIdent(label1), ZERO_STR, tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

string IRBuilder::addItemSetAfterCompare(IROp op, string& label1, string& label2) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        string tmpSymbol = genTmpSymbol();
        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tmpSymbol));
        return tmpSymbol;
    }
    return "";
}

void IRBuilder::addItemBranchAfterCompare(IROp op, string& label1, string&& label2, string&& tarLabelName) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tarLabelName));
    }
}

void IRBuilder::addItemBranchAfterCompare(IROp op, string& label1, string&& label2, string& tarLabelName) {
    if (inEffect) {
        freeTmpSymbol(label1);
        freeTmpSymbol(label2);

        IRs.emplace_back(IRItem(op, markUniqueIdent(label1), markUniqueIdent(label2), tarLabelName));
    }
}

void IRBuilder::addItemLabel(string& labelName) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::LABEL, NULL_STR, NULL_STR, labelName));
    }
}

void IRBuilder::addItemJump(string &labelName) {
    if (inEffect) {
        IRs.emplace_back(IRItem(IROp::JUMP, NULL_STR, NULL_STR, labelName));
    }
}