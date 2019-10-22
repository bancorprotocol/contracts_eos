
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>

#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <math.h>
#include "events.hpp"

using namespace eosio;
using namespace std;

typedef vector<string> path;

struct converter {
    name   account;
    string sym;
};

struct memo_structure {
    path              path;
    vector<converter> converters;   
    string            version;
    string            min_return;
    string            dest_account;
    string            receiver_memo;
};

#define BANCOR_X "bancorxoneos"_n
#define BANCOR_NETWORK "thisisbancor"_n
#define BNT_TOKEN "bntbntbntbnt"_n

vector<string> split(const string& str, const string& delim) {
    vector<string> tokens;
    size_t prev = 0, pos = 0;

    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}

std::string build_memo(memo_structure data) {
    std::string pathstr = "";
    for(auto i=0; i < data.path.size(); i++){
        if(i != 0)
            pathstr.append(" ");
        pathstr.append(data.path[i]);
    }

    std::string memo = "";
    memo.append(data.version);
    memo.append(",");
    memo.append(pathstr);
    memo.append(",");
    memo.append(data.min_return);
    memo.append(",");
    memo.append(data.dest_account);
    memo.append(";");
    memo.append(data.receiver_memo);
    return memo;
}

/** @dev to_fixed 
 *  formats a number to a fixed precision
 *  e.g. - to_fixed(14.214212, 3) --> 14.214
*/
double to_fixed(double num, int precision) {
    return (int)(num * pow(10, precision)) / pow(10, precision);
}

memo_structure parse_memo(std::string memo) {
    auto res = memo_structure();
    auto split_memos = split(memo, ";"); // we separate concantenated memos with ";"
    auto parts = split(split_memos[0], ","); // split the first memo by ","

    res.version = parts[0];

    auto path_elements = split(parts[1], " ");

    if (path_elements.size() == 1 && path_elements[0] == "") {
        res.path = {};
    } else
        res.path = path_elements;

    res.converters = {};
    for (int i = 0; i < res.path.size(); i += 2) {
        auto converter_data = split(res.path[i], ":");
        
        auto cnvrt = converter();
        cnvrt.account = name(converter_data[0].c_str());
        cnvrt.sym = converter_data.size() > 1 ? converter_data[1] : "";
        res.converters.push_back(cnvrt);
    }

    if (split_memos.size() == 2) {
        res.receiver_memo = split_memos[1];
    } else
        res.receiver_memo = "convert"; // default memo for receiver account

    res.min_return = parts[2];
    res.dest_account = parts[3];

    return res;
}

