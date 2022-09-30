//
// Created by Excalibur on 2022/9/12.
//
#include <iostream>
#include <fstream>
#include <cstdio>
#include "lexer.h"
#include "parser.h"

using namespace std;

int main() {
    ifstream ifs;

    ifs.open("testfile.txt", ios::in);
    if (ifs.fail()) {
        cerr << "failed to read!" << endl;
        return -1;
    }

    doLexicalAnalysis(ifs);
    ifs.close();
    doGrammarAnalysis();

    return 0;
}