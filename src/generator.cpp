//
// Created by Excalibur on 2022/11/4.
//

#include "generator.h"
#include "irbuilder.h"
#include "lexer.h"
#include <vector>

using namespace std;


void doMipsGeneration() {
    Generator generator(IRList);
    // generator.printMIPS();
}


Generator::Generator(vector<IRItem>& ir): srcIR(ir), pos(1), peek(ir[0]) {
    textSeg.emplace_back("li $fp, 0x10040000");
    textSeg.emplace_back("j main");
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
        ofs << inst << endl;
    }
}

IRItem Generator::nextIR() {
    if (pos < srcIR.size()) {
        peek = srcIR[pos++];
    }
    return peek;
}

IRItem Generator::prevIR() {
    if (pos > 1) {
        peek = srcIR[pos-2];
        pos = pos - 1;
    }
    return peek;
}

void Generator::generating() {
    while (!hitEnd()) {
        switch (peek.op) {

            default:
                break;
        }

        nextIR();
    }
}

void Generator::genConst() {
    if (stoi(peek.label2) == 0) {

    }
}