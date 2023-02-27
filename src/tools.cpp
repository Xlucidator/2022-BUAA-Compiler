//
// Created by Excalibur on 2022/11/4.
//

#include "tools.h"

std::vector<std::string> split(const std::string& str, const std::string& delimiters) {
    std::vector<std::string> res;
    std::string::size_type begin = 0, end = str.find(delimiters);

    while (end != std::string::npos) {
        res.emplace_back(str.substr(begin, end-begin));
        // new round
        begin = end + delimiters.size();
        end = str.find(delimiters, begin);
    }
    if (begin < str.length()){
        res.emplace_back(str.substr(begin));
    } else {
        res.emplace_back("");
    }

    return res;
}

int calculate(std::string& num1, IROp op, std::string& num2) {
    int res = 0;
    switch (op) {
        case IROp::ADD: res = stoi(num1) + stoi(num2); break;
        case IROp::MIN: res = stoi(num1) - stoi(num2); break;
        case IROp::MUL: res = stoi(num1) * stoi(num2); break;
        case IROp::DIV: res = stoi(num1) / stoi(num2); break;
        case IROp::MOD: res = stoi(num1) % stoi(num2); break;
        case IROp::BITAND: res = stoi(num1) & stoi(num2); break;
        default: ;
    }
    return res;
}

