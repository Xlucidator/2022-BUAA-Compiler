//
// Created by Excalibur on 2022/9/12.
//
#include <iostream>
#include <fstream>
#include <cstdio>
#include "lexer.h"

using namespace std;

int main() {
    ifstream ifs;

    ifs.open("../test/testfile.txt", ios::in);
    if (ifs.fail()) {
        cerr << "failed to read!" << endl;
        return -1;
    }

    doLexicalAnalysis(ifs);

    ifs.close();

    return 0;
}