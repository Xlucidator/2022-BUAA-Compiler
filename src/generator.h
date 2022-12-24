//
// Created by Excalibur on 2022/11/4.
//

#ifndef COMPILER_GENERATOR_H
#define COMPILER_GENERATOR_H


#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "irbuilder.h"
#include "tools.h"
#include "settings.h"

using namespace std;

#define SREG_SIZE (8)
#define TREG_SIZE (10)

void doMipsGeneration();


struct RecordItem {
    int dim;
    bool is_pointer;
    string reg;
    string base_reg;
    int addr_off;
};


class Generator {
private:
    /* about IR code */
    vector<IRItem>& srcIR;
    int pos;
    IRItem peek;

    /* about const & .word */
    map<string, string> constRecords;       // <const_name, const_value>
    set<string> wordLabel;
    map<string, string> constAsciiLabel;    // <content, label>
    int constStringNo = 0;

    /* about register & array-memory-addr */
    map<string, RecordItem> records;        // <name, recordItem>

    bitset<10> t_map;
    bitset<8>  s_map;
    string t_reg[TREG_SIZE];
    string s_reg[SREG_SIZE];
    int t_cnt = 0;
    int s_cnt = 0;

    int fp_offset = 0;
    int gp_offset = 0;
    int sp_offset = 0;

    /* about mips code */
    vector<string> dataSeg;
    vector<string> textSeg;
    ofstream ofs;

    inline bool hitEnd() {
        return pos == srcIR.size();
    }

public:
    const map<IROp, string> IROp2String = {
            {IROp::ADD, "add"},
            {IROp::MIN, "sub"},
            {IROp::MUL, "mul"},
            {IROp::DIV, "div"},

            {IROp::SEQ, "seq"},
            {IROp::SNE, "sne"},
            {IROp::SLT, "slt"},
            {IROp::SLE, "sle"},
            {IROp::SGT, "sgt"},
            {IROp::SGE, "sge"},

            {IROp::BEQ, "beq"},
            {IROp::BNE, "bne"},
    };

    explicit Generator(vector<IRItem>& ir);
    inline IRItem nextIR();
    inline IRItem prevIR();
    inline IRItem preLook(int offset);

    void printMIPS();
    void generating();

    /* generate part */
    void genConst();
    void genLocalConst();
    void genGlobalVar();
    void genLocalVar();
    void genFuncDef();
    void genStmt();
    void genFuncCall();
    void genPrintf();
    void genScanf();

    inline int getRegNo(string& regStr);
    /* record & reg part */
    void resetFPOffset() {
        fp_offset = 0;
    }

    void resetRegFile() {
        for (string& reg : s_reg)
            reg = "";
        for (string& reg : t_reg)
            reg = "";
    }

    void addConstValue(string& name, string& value) {
        // TODO: const name will be different
        constRecords[name] = value;
    }

    void addWordLabel(string& arrayName) {  /* for array, const or global */
        wordLabel.emplace(arrayName);
    }

    string initRegisterVar(string& name, string&& baseReg, int addrOff) {
        // return allocReg
        RecordItem newItem;
        newItem.is_pointer = false;
        newItem.reg = allocReg(name);
        newItem.base_reg = baseReg;
        newItem.addr_off = addrOff;
        records[name] = newItem;

        return newItem.reg;
    }

    string initGlobalVar(string& name) {
        // return allocReg
        string initReg = initRegisterVar(name, "$gp", gp_offset);
        gp_offset += 4;
        return initReg;
    }

    string initLocalVar(string& name) {
        // return allocReg
        string initReg = initRegisterVar(name, "$fp", fp_offset);
        fp_offset += 4;
        return initReg;
    }

    int initLocalArray(string& name, int len) {
        RecordItem newItem;
        newItem.is_pointer = false;
        // doesn't need corresponding reg
        newItem.base_reg = "$fp";
        newItem.addr_off = fp_offset;
        records[name] = newItem;

        fp_offset += 4 * len;
        return newItem.addr_off;
    }

    void initLocalPointer(string& name) {
        RecordItem newItem;
        newItem.is_pointer = true;
        newItem.reg = allocReg(name);   // should have reg to store the addr
        newItem.base_reg = "$fp";
        newItem.addr_off = fp_offset;
        records[name] = newItem;

        fp_offset += 4;
    }

    void saveBackToMemory(string& tarReg) {
        string* regs = (tarReg[1] == 's') ? s_reg : t_reg;
        string tarRecordName = regs[getRegNo(tarReg)];  // get record name
        // do only if it is not in memory
        if (!tarRecordName.empty()) {
            RecordItem tarRecord = records[regs[getRegNo(tarReg)]]; // get record
            // save back
            string sw_inst = "sw " + tarReg + " " +
                             to_string(tarRecord.addr_off) + "(" + tarRecord.base_reg + ")";
            textSeg.emplace_back(sw_inst);
            // clear reg mark
            regs[getRegNo(tarReg)] = "";
        }
    }

    void loadFromMeory(string& tarRecordName) {
        RecordItem tarRecord = records[tarRecordName];
        string tarReg = tarRecord.reg;
        string* regs = (tarReg[1] == 's') ? s_reg : t_reg;
        // load
        string lw_inst = "lw " + tarReg + " " +
                to_string(tarRecord.addr_off) + "(" + tarRecord.base_reg + ")";
        textSeg.emplace_back(lw_inst);
        // mark in used reg
        regs[getRegNo(tarReg)] = tarRecordName;
    }

    void storeAllGlobalVarBack() {
        // global variable need to store back before function call
        for (int i = 0; i < SREG_SIZE; ++i) {
            string& lingerRecord = s_reg[i];
            if (!lingerRecord.empty()) {
                RecordItem item = records[lingerRecord];
                if (item.base_reg == "$gp") {
                    // it means global variable
                    string tarReg = "$s" + to_string(i);
                    saveBackToMemory(tarReg);
                }
            }
        }
    }

    void clearSRegFileAtEndOfBlock() {
        // TRegFile need not, currently
        for (int i = 0; i < SREG_SIZE; ++i) {
            if (s_reg[i].empty())
                continue;
            string tarReg = "$s" + to_string(i);
            saveBackToMemory(tarReg);
        }
    }

    void clearConflict(string& tarRecordName, string& tarReg, bool needValue) {
        /* before recordName want to use tarReg */
        string* regs = (tarReg[1] == 's') ? s_reg : t_reg;
        int tarRecordNo = getRegNo(tarReg);

        if (regs[tarRecordNo] != tarRecordName) {
            /* tarRecord isn't in tarReg, need to load it to tarReg */
            RecordItem tarRecord = records[tarRecordName];

            if (!regs[tarRecordNo].empty()) {
                /* tarReg is not empty, need to store old one to memory */
                RecordItem conflictRecord = records[regs[tarRecordNo]];
                string sw_inst = "sw " + tarReg + " " +
                        to_string(conflictRecord.addr_off) + "(" + conflictRecord.base_reg + ")";
                textSeg.emplace_back(sw_inst);
            }

            if (needValue) { // use as rvalue, so we should load its value to reg
                string lw_inst = "lw " + tarReg + " " +
                                 to_string(tarRecord.addr_off) + "(" + tarRecord.base_reg + ")";
                textSeg.emplace_back(lw_inst);
            } // or we simply regard the same reg as a new record (do not care what the value is)

            regs[tarRecordNo] = tarRecordName;
        }
    }

    string useName(string& name, bool needValue) {
        if (isnumber(name)) {
            // number, use number
            return name;
        } else if (constRecords.find(name) != constRecords.end()) {
            // const value, use number
            return constRecords[name];
        } else {
            // variable, use register
            return useNameFromReg(name, needValue);
        }
    }

    string useNameFromReg(string& name, bool needValue) {
        /* return reg that is to use represent the name */
        string tarReg;
        if (isreg(name)) {
            throw "cannot pass register as variable name";
        }

        if (isnumber(name)) {
            // number, will always need value
            if (name == "0") tarReg = "$0";
            else {
                string inst = "li $t9 " + name;
                textSeg.emplace_back(inst);
                tarReg = "$t9";
            }
        } else if (constRecords.find(name) != constRecords.end()) {
            // const value, will always need value
            string value = constRecords[name];
            if (value == "0") tarReg = "$0";
            else {
                string inst = "li $t9 " + value;
                textSeg.emplace_back(inst);
                tarReg = "$t9";
            }
        } else {
            // variable
            auto iter = records.find(name);
            if (iter == records.end()) {
                // record not exist -> alloc one
                tarReg = initRegisterVar(name, "$fp", fp_offset);
                fp_offset += 4;
            } else {
                // record exist
                RecordItem item = records[name];
                tarReg = item.reg;
            }
            clearConflict(name, tarReg, needValue);    // name: want to use tarReg
        }

        return tarReg;
    }
    // Overload
    string useNameFromReg(string& name, bool needValue, int chooseReg) {
        /* return reg that is to use represent the name */
        string tarReg;
        if (isreg(name)) {
            throw "cannot pass register as variable name";
        }

        if (isnumber(name)) {
            // number, will always need value
            if (name == "0") tarReg = "$0";
            else {
                string numReg = "$t" + to_string(chooseReg);
                string inst = "li " + numReg + " " + name;
                textSeg.emplace_back(inst);
                tarReg = numReg;
            }
        } else if (constRecords.find(name) != constRecords.end()) {
            // const value, will always need value
            string value = constRecords[name];
            if (value == "0") tarReg = "$0";
            else {
                string inst = "li $t9 " + value;
                textSeg.emplace_back(inst);
                tarReg = "$t9";
            }
        } else {
            // variable
            auto iter = records.find(name);
            if (iter == records.end()) {
                // record not exist -> alloc one
                tarReg = initRegisterVar(name, "$fp", fp_offset);
                fp_offset += 4;
            } else {
                // record exist
                RecordItem item = records[name];
                tarReg = item.reg;
            }
            clearConflict(name, tarReg, needValue);    // name: want to use tarReg
        }

        return tarReg;
    }

    string useArrayFromBaseOff(string& name, string& index) {
        /* a[t] means: name - 'a'  index - 't' */
        string baseOff;     // off(base)

        // name is from $fp / $gp
        if (records.find(name) != records.end()) {
            RecordItem item = records.at(name);
            if (!item.is_pointer) {// $fp/$gp + offset

                if (isnumber(index)) {      // like [3]
                    int offset = 4 * stoi(index) + item.addr_off;
                    baseOff = to_string(offset) + "(" + item.base_reg + ")";
                } else {                    // like [@t4]
                    string base = useNameFromReg(index, true, 7);
                    textSeg.emplace_back("sll $t8 " + base + " 2");
                    textSeg.emplace_back("add $t8 $t8 " + item.base_reg);
                    baseOff = to_string(item.addr_off) + "($t8)";
                }
                return baseOff;

            } else {    // pointer

                string pointerReg = useNameFromReg(name, true, 7);
                if (isnumber(index)) {
                    int offset = 4 * stoi(index);
                    baseOff = to_string(offset) + "(" + pointerReg + ")";
                } else {
                    string base = useNameFromReg(index, true, 7);
                    textSeg.emplace_back("sll $t8 " + base + " 2");
                    textSeg.emplace_back("add $t8 $t8 " + pointerReg);
                    baseOff = "0($t8)";
                }
                return baseOff;

            }
        }

        // name is from .word
        if (wordLabel.find(name) != wordLabel.end()) {
            string base;    // must be reg '$xx'
            if (isnumber(index)) {
                string offset = to_string(4 * stoi(index));
                base = useNameFromReg(offset, true, 7);
            } else {
                base = useNameFromReg(index, true, 7);
                textSeg.emplace_back("sll $t8 " + base + " 2"); // << 2, means *4
                base = "$t8";
            }
            baseOff = name + "(" + base + ")";
            return baseOff;
        }

        cout << "name:[" << name << "] is undefined" << endl;
        return "";
    }

    string useArrayFromAddr(string& name, string& index) {
        string addr;

        // deal with name
        if (records.find(name) != records.end()) {
            // name is from $fp / $gp
            RecordItem item = records.at(name);

            if (!item.is_pointer) { // $fp/$gp + offset
                textSeg.emplace_back("add $t8 " + item.base_reg + " " + to_string(item.addr_off));
            } else {                // pointer
                string pointerReg = useNameFromReg(name, true, 7);
                textSeg.emplace_back("move $t8 " + pointerReg);
            }

        } else if (wordLabel.find(name) != wordLabel.end()) {
            // name is from .word
            textSeg.emplace_back("la $t8 " + name);
        } else {
            cout << "name:[" << name << "] is undefined" << endl;
            return "";
        }

        // deal with index : *4
        string indexReg;
        if (isnumber(index)) {
            string indexOffset = to_string(4 * stoi(index));
            indexReg = useNameFromReg(indexOffset, true, 7);
        } else {
            // index ident may always be @tx - tmp
            if (!istmp(index)) {
                cout << "err may occur" << endl;
            }
            indexReg = useNameFromReg(index, true, 7);
            textSeg.emplace_back("sll " + indexReg + " " + indexReg + " 2");
        }

        // add name + index together
        textSeg.emplace_back("add $t8 $t8 " + indexReg);

        return "$t8";
    }

    string allocReg(string& name) {
        return (name[0] == '@') ? allocTmpReg(name) : allocStoreReg(name);
    }

    string allocStoreReg(string& recordName) {
        /* select one reg that is not allocated */
        for (int reg_no = 0; reg_no < SREG_SIZE; ++reg_no) {
            if (!s_map[reg_no]) {
                s_map[reg_no] = true;
                return "$s" + to_string(reg_no);
            }
        }
        /* select one reg that is not used */
            // TODO: select one reg that is not in use
        /* select in order */
        int reg_no = s_cnt;
        s_cnt = (s_cnt + 1) % SREG_SIZE;
        return "$s" + to_string(reg_no);
    }

    string allocTmpReg(string& recordName) {
        /* select one reg that is not allocated */
        // $t9 is for number, $t8 is for index*4, $t7 is for baseOff-num : -3
        for (int reg_no = 0; reg_no < TREG_SIZE-3; ++reg_no) {
            if (!t_map[reg_no]) {
                t_map[reg_no] = true;
                return "$t" + to_string(reg_no);
            }
        }
        /* select one reg that is not used */
        // TODO: select one reg that is not in use
        /* select in order */
        int reg_no = t_cnt;
        t_cnt = (t_cnt + 1) % (TREG_SIZE-3);
        return "$t" + to_string(reg_no);
    }

};




inline IRItem Generator::nextIR() {
    if (pos < srcIR.size()) {
        peek = srcIR[pos++];
    }
    return peek;
}

inline IRItem Generator::prevIR() {
    if (pos > 1) {
        peek = srcIR[pos-2];
        pos = pos - 1;
    }
    return peek;
}

inline IRItem Generator::preLook(int offset) {
    if (pos + offset - 1 < srcIR.size()) {
        return srcIR[pos + offset - 1];
    }
    cout << "preLook out of bound" << endl;
    return srcIR[pos];
}

inline int Generator::getRegNo(string& regStr) {
    return stoi(regStr.substr(2));
}

#endif //COMPILER_GENERATOR_H
