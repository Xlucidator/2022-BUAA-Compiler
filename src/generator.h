//
// Created by Excalibur on 2022/11/4.
//

#ifndef COMPILER_GENERATOR_H
#define COMPILER_GENERATOR_H


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "irbuilder.h"
#include "lexer.h"

using namespace std;


void doMipsGeneration();


class Generator {
private:
    vector<IRItem>& srcIR;
    int pos;
    IRItem peek;

    vector<string> dataSeg;
    vector<string> textSeg;
    ofstream ofs;

    inline bool hitEnd() {
        return pos == srcIR.size();
    }

public:
    explicit Generator(vector<IRItem>& ir);
    IRItem nextIR();
    IRItem prevIR();

    void printMIPS();
    void generating();

    void genConst();
};


#endif //COMPILER_GENERATOR_H
