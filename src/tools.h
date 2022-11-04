//
// Created by Excalibur on 2022/11/2.
//

#ifndef COMPILER_TOOLS_H
#define COMPILER_TOOLS_H

#include <string>
#include <vector>
#include "irbuilder.h"

/* Usage:
 * str = "%dabcd%d%d efg%d"
 * split(str, "%d") => ["", "abcd", "", " efg", ""]
 *
 * number of "," will be the number of delimiters
 * */
std::vector<std::string> split(const std::string& str, const std::string& delimiters = " ");

inline bool hasSign(std::string& str) {
    return str[0] == '-';
}

inline bool isnumber(std::string& str) {
    if (str[0] == '-')
        return isdigit(str[1]);
    return isdigit(str[0]);
}

int calculate(std::string& num1, IROp op, std::string& num2);

#endif //COMPILER_TOOLS_H
