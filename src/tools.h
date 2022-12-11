//
// Created by Excalibur on 2022/11/2.
//

#ifndef COMPILER_TOOLS_H
#define COMPILER_TOOLS_H

#include <string>
#include <vector>
#include "symbol.h"
#include "irbuilder.h"

/* Usage:
 * str = "%dabcd%d%d efg%d"
 * split(str, "%d") => ["", "abcd", "", " efg", ""]
 *
 * number of "," will be the number of delimiters
 * */
std::vector<std::string> split(const std::string& str, const std::string& delimiters = " ");

int calculate(std::string& num1, IROp op, std::string& num2);

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

inline bool istmp(std::string& str) {
    if (str[0] == '-')
        return str.substr(1,2) == "@t";
    return str.substr(0,2) == "@t";
}

inline bool isformatstr(std::string& str) {
    return str[0] == '\"';
}

inline std::string getArrayIndex(std::string& str) {
    std::string::size_type begin = str.find('['), end = str.find(']');
    return str.substr(begin+1, end-begin-1);
}

inline std::string getArrayIdent(std::string& str) {
    return str.substr(0, str.find('['));
}

// has no side effect
inline std::string stripQuot(std::string& str) {
    if (str[0] != '\"')
        return str;
    return str.substr(1, str.length()-2);
}

// has no side effect
inline std::string markUniqueIdent(std::string& str) {
    if (isnumber(str) || istmp(str) || isformatstr(str) || str == "RET" || str.find('#') != std::string::npos) {
        // number, tmpVar, has been marked  -> do not modify
        return str;
    }
    int identTableNo = curContext->locateIdentTableNo(str);
    if (identTableNo == 0)   // global table -> do not modify
        return str;
    return str + "#" + to_string(identTableNo);
}

#define TO_LABEL_BEGIN(endStr) (endStr.replace(endStr.find("end"), 3, "begin"))

#endif //COMPILER_TOOLS_H
