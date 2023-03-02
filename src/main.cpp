//
// Created by Excalibur on 2022/9/12.
//
#include <iostream>
#include <fstream>
#include <cstdio>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "generator/generator.h"

using namespace std;

int main() {
    doLexicalAnalysis();
    doSyntaxAnalysis();
    doMipsGeneration();
    return 0;
}