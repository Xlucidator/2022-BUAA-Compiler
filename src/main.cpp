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
    doLexicalAnalysis();
    doSyntaxAnalysis();

    return 0;
}