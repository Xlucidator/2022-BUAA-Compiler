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
    string name;
    int dim;
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
    map<string, string> constRecords;
    vector<string> wordLabel;
    map<string, string> constAsciiLabel;    // < content , label >
    int constStringNo = 0;

    /* about register */
    map<string, RecordItem> records;

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
    explicit Generator(vector<IRItem>& ir);
    inline IRItem nextIR();
    inline IRItem prevIR();
    inline IRItem preLook(int offset);

    void printMIPS();
    void generating();

    /* generate part */
    void genConst();
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

    void addWordLabel(string& arrayName) {
        wordLabel.push_back(arrayName);
    }

    void clearRecords() {
        for (auto& record : records) {
            if (record.second.base_reg != "$gp") {

            }
        }
    }

    string initRegisterVar(string& name, string&& baseReg, int addrOff) {
        // return allocReg
        RecordItem newItem;
        newItem.name = name;
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
        // $t9 is for number
        for (int reg_no = 0; reg_no < TREG_SIZE-1; ++reg_no) {
            if (!t_map[reg_no]) {
                t_map[reg_no] = true;
                return "$t" + to_string(reg_no);
            }
        }
        /* select one reg that is not used */
        // TODO: select one reg that is not in use
        /* select in order */
        int reg_no = t_cnt;
        t_cnt = (t_cnt + 1) % (TREG_SIZE-1);
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
