//
// Created by Excalibur on 2022/11/4.
//

#include "generator.h"
#include "irbuilder.h"
#include <vector>

using namespace std;

#define USE_FROM true   // needValue = true, where the value comes from
#define USE_TO false    // needValue = false, where the value comes to

void doMipsGeneration() {

    Generator generator(IRList);
    generator.generating();

    generator.printMIPS();
}


Generator::Generator(vector<IRItem>& ir): srcIR(ir), pos(1), peek(ir[0]) {
    ofs.open("mips.txt", ios::out);
    if (ofs.fail()) {
        cerr << "failed to write!" << endl;
        return;
    }
}

void Generator::printMIPS() {
    ofs << ".data" << endl;
    for (auto& inst : dataSeg) {
        ofs << inst << endl;
    }

    ofs << endl << ".text" << endl;
    for (auto& inst : textSeg) {
        if (inst.find(':') != string::npos) {
            ofs << endl;
        }
        ofs << inst << endl;
    }
}


void Generator::generating() {
    while (peek.op == IROp::DEF_CON || peek.op == IROp::DEF_VAR) {
        if (peek.op == IROp::DEF_CON) genConst();
        else genGlobalVar();
    }

    textSeg.emplace_back("");
    textSeg.emplace_back("li $fp, 0x10040000");
    textSeg.emplace_back("j main");

    while (peek.op == IROp::DEF_FUN ) {
        genFuncDef();
    }

    cout << "finish generation" << endl;
}

void Generator::genConst() {
    /* peek.op == IROp::DEF_CON */
    string constName = peek.res;
    if (peek.label1 != "array") {   // not an array (a[0]?, a[1])
        if (nextIR().op != IROp::DEF_INIT)
            cerr << constName << " should have DEF_INIT" << endl;
        string constValue = peek.label1;
        addConstValue(constName, constValue);
        nextIR();
    } else {
        string inst = constName + ": .word ";
        while (nextIR().op != IROp::DEF_END) {
            inst += peek.label1 + " ";
        }
        addWordLabel(constName);
        dataSeg.emplace_back(inst);
    }
    // cur op: DEF_END
    nextIR(); // move next
}

void Generator::genGlobalVar() {
    /* peek.op == IROp::DEF_VAR */
    string varName = peek.res;
    if (peek.label1 != "array") {
        // not array, we should store it above $gp
        string initReg = initGlobalVar(varName);
        clearConflict(varName, initReg, false);
        string initValue = "0"; // default initialization: 0
        if (nextIR().op == IROp::DEF_INIT) {
            // has initialization
            initValue = peek.label1;
            nextIR();
        }
        string li_inst = "li " + initReg + " " + initValue;
        textSeg.emplace_back(li_inst);
        // saveBackToMemory(initReg);
    } else {
        string inst = varName + ": .word ";
        if (preLook(1).op != IROp::DEF_INIT) {
            // has no initialization, so we init them as 0
            int indexSize = stoi(peek.label2);
            for (int i = 0; i < indexSize; ++i) {
                inst += "0 ";
            }
        } else {
            // has full initialization
            while (nextIR().op != IROp::DEF_END) {
                inst += peek.label1 + " ";
            }
        }
        addWordLabel(varName);
        dataSeg.emplace_back(inst);
    }
    // cur op: DEF_END
    nextIR();
}

void Generator::genLocalVar() {
    /* peek.op == IROp::DEF_VAR */
    string varName = peek.res;
    if (peek.label1 != "array") {
        /* not array, we should store it above $fp */
        string initReg = initLocalVar(varName);
        if (nextIR().op == IROp::DEF_INIT) {
            // has initialization
            string initValue = peek.label1;
            clearConflict(varName, initReg, false);
            string li_inst = "li " + initReg + " " + initValue;
            textSeg.emplace_back(li_inst);
            nextIR();
        }
    } else {
        /* array, but we alloc and store it in $fp */
        // TODO: haven't equipped
        cout << "local var array is not equipped" << endl;
    }
    // cur op: DEF_END
    nextIR();
}

void Generator::genFuncDef() {
    /* peek.op == IROp::DEF_FUNC */
    resetFPOffset();    // switch to new function frame $fp
    resetRegFile();     // new function should always have clear context
    string funcName = peek.res;
    textSeg.push_back(funcName + ":");

    int FPara_cnt = 0;
    while (nextIR().op == IROp::FPARA) {
        string paraName = peek.res;
        useNameFromReg(paraName, true); // TODO: not so clear, change to init... function

        FPara_cnt += 1;
    }

    while (peek.op != IROp::DEF_END || peek.res != funcName) {
        genStmt();
    }

    if (funcName != "main")
        textSeg.emplace_back("jr $ra");
    // cur op : DEF_END
    nextIR();
}

void Generator::genStmt() {
    string inst;
    switch(peek.op) {
        case IROp::DEF_CON: {
            genConst();
            break;
        }
        case IROp::DEF_VAR: {
            genLocalVar();
            break;
        }
        case IROp::ADD: {
            string rd = useNameFromReg(peek.res, USE_TO);
            string rs = useNameFromReg(peek.label1, USE_FROM);
            string rt = useName(peek.label2, USE_FROM);
            inst = "add " + rd + " " + rs + " " + rt;
            textSeg.emplace_back(inst);
            nextIR();
            break;
        }
        case IROp::MIN: {
            string rd = useNameFromReg(peek.res, USE_TO);
            string rs = useNameFromReg(peek.label1, USE_FROM);
            string rt = useName(peek.label2, USE_FROM);
            inst = "sub " + rd + " " + rs + " " + rt;
            textSeg.emplace_back(inst);
            nextIR();
            break;
        }
        case IROp::MUL: {
            string rd = useNameFromReg(peek.res, USE_TO);
            string rs = useNameFromReg(peek.label1, USE_FROM);
            string rt = useName(peek.label2, USE_FROM);
            inst = "mul " + rd + " " + rs + " " + rt;   // rd will get low-32bit of the res
            textSeg.emplace_back(inst);
            nextIR();
            break;
        }
        case IROp::DIV: {
            string rd = useNameFromReg(peek.res, USE_TO);
            string rs = useNameFromReg(peek.label1, USE_FROM);
            string rt = useName(peek.label2, USE_FROM);
            inst = "div " + rd + " " + rs + " " + rt;   // rd will get low-32bit of the res
            textSeg.emplace_back(inst);
            nextIR();
            break;
        }
        case IROp::MOD: {
            string rd = useNameFromReg(peek.res, USE_TO);
            string rs = useNameFromReg(peek.label1, USE_FROM);
            string rt = useName(peek.label2, USE_FROM);
            inst = "div " + rs + " " + rt;              // rd will get high-32bit of the res (HI)
            textSeg.emplace_back(inst);
            inst = "mfhi " + rd;
            textSeg.emplace_back(inst);
            nextIR();
            break;
        }
        case IROp::PRINTF: {
            genPrintf();
            break;
        }
        case IROp::SCANF: {
            genScanf();
            break;
        }
        case IROp::CALL_FUN:
        case IROp::RPARA: {
            genFuncCall();
            break;
        }
        case IROp::RET: {
            string from = useNameFromReg(peek.label1, USE_FROM);
            inst = "move $v0 " + from;
            textSeg.emplace_back(inst);
            nextIR();
        }

        default:
            break;
    }
}

void Generator::genFuncCall() {
    /* peek.op == IROp::CALL_FUN || IROp::RPARA */
    textSeg.emplace_back("");

    /* load RPara */
    int fp_offset_record = fp_offset;
    while (peek.op == IROp::RPARA) {
        string rt = useNameFromReg(peek.label1, USE_FROM);
        string sw_inst = "sw " + rt + " " + to_string(fp_offset) + "($fp)";
        fp_offset += 4;
        textSeg.emplace_back(sw_inst);
        nextIR();
    }
    // cur op: CALL_FUN
    fp_offset = fp_offset_record;
    textSeg.emplace_back("");

    /* get something to store */
    vector<string> backUpList;  // list which is to sw to the addr of $sp
    string content;
    for (int i = 0; i < SREG_SIZE; ++i) {
        if (!s_reg[i].empty()) {
            // cout << s_reg[i] << endl;
            content = "$s" + to_string(i) + " " + to_string(sp_offset) + "($sp)";
            sp_offset += 4;
            backUpList.emplace_back(content);
            // s_reg[i] = "";
        }
    }
    for (int i = 0; i < TREG_SIZE-1; ++i) {
        if (!t_reg[i].empty()) {
            // cout << t_reg[i] << endl;
            content = "$t" + to_string(i) + " " + to_string(sp_offset) + "($sp)";
            sp_offset += 4;
            backUpList.emplace_back(content);
            // t_reg[i] = "";
        }
    }
    content = "$ra " + to_string(sp_offset) + "($sp)";
    sp_offset += 4;

    /* store context */
    textSeg.emplace_back("addi $sp $sp " + to_string(-sp_offset));
    for (string& storeContent : backUpList) {
        textSeg.emplace_back("sw " + storeContent);
    }

    /* call function */
    string callFuncName = peek.label1;
    textSeg.emplace_back("addi $fp $fp " + to_string(fp_offset));
    textSeg.emplace_back("jal " + callFuncName);
    textSeg.emplace_back("addi $fp $fp " + to_string(-fp_offset));
    nextIR();
    // cur op： after CALL_FUN, unknown

    /* load context */
    for (string& storeContent : backUpList) {
        textSeg.emplace_back("lw " + storeContent);
    }
    textSeg.emplace_back("addi $sp $sp " + to_string(sp_offset));
    textSeg.emplace_back("");

    /* get return value (if has) */
    if (peek.label1 == "RET") {
        string rd = useNameFromReg(peek.res, USE_TO);
        textSeg.emplace_back("move " + rd + " $v0");
        nextIR();
    }

    sp_offset = 0;
}

void Generator::genPrintf() {
    textSeg.emplace_back("");
    /* peek.op == IROp::PRINTF */
    string content = peek.label1;
    if (content[0] == '\"') {
        // content : "xxx"
        string conStr = stripQuot(content);
        string labelName;
        if (constAsciiLabel.find(conStr) == constAsciiLabel.end()) {
            // not exist
            labelName = "CONSTR_4_PRINTF_" + to_string(constStringNo++);
            dataSeg.emplace_back(labelName + ":" + " .asciiz " + content); // content: include quot
            constAsciiLabel[conStr] = labelName;
        } else {
            // exist
            labelName = constAsciiLabel[conStr];
        }
        textSeg.emplace_back("la $a0 " + labelName);
        textSeg.emplace_back("li $v0 4");
    } else {
        // content : a , 1
        string printReg = useNameFromReg(content, USE_FROM);
        textSeg.emplace_back("move $a0 " + printReg);
        textSeg.emplace_back("li $v0 1");
    }
    textSeg.emplace_back("syscall");
    textSeg.emplace_back("");
    nextIR();
}

void Generator::genScanf() {
    textSeg.emplace_back("");
    /* peek.op == IROp::SCANF */
    string scanfReg = useNameFromReg(peek.label1, USE_TO);
    textSeg.emplace_back("li $a0 5");
    textSeg.emplace_back("syscall");
    textSeg.emplace_back("move " + scanfReg + " $v0");

    textSeg.emplace_back("");
    nextIR();
}