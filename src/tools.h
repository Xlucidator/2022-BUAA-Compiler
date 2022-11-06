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

inline void removeSign(std::string& str) {
    if (str[0] == '-') {
        str = str.substr(1);
    }
}

inline bool isnumber(std::string& str) {
    if (str[0] == '-')
        return isdigit(str[1]);
    return isdigit(str[0]);
}

inline bool isarray(std::string& str) {
    return str.find('[') != std::string::npos;
}

inline string getArrayIndex(std::string& str) {
    std::string::size_type begin = str.find('['), end = str.find(']');
    return str.substr(begin+1, end-begin-1);
}

inline string getArrayIdent(std::string& str) {
    return str.substr(0, str.find('['));
}

inline string stripQuot(std::string& str) {
    if (str[0] != '\"')
        return str;
    return str.substr(1, str.length()-2);
}

int calculate(std::string& num1, IROp op, std::string& num2);

#endif //COMPILER_TOOLS_H
