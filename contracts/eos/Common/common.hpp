#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>

#include <string>
#include <vector>
#include <iterator>
#include "events.hpp"

using std::string;
using std::vector;

vector<string> split(const string& str, const string& delim)
{
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

using namespace eosio;

typedef std::vector<std::string> path;

struct memo_structure {
    std::string convert_memo;
    std::string receiver_memo;
};

struct memo_convert_structure {
    path    path;
    std::string version;
    std::string to_token;
    std::string min_return;
};

std::string build_memo(memo_convert_structure data) {
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
    memo.append(data.to_token);
    return memo;
}

memo_structure parse_memo(std::string memo) {
    auto res = memo_structure();
    auto split_memos = split(memo, ";"); // we separate concantenated memos with ";"

    eosio::print("split memos length: ");
    eosio::print(split_memos.size());
    eosio::print("\n");

    res.convert_memo = split_memos[0];
    if (split_memos.size() == 2) {
        res.receiver_memo = split_memos[1];
    } else {
        res.receiver_memo = "convert";
    }

    return res;
}

memo_convert_structure parse_convert_memo(std::string memo){
    auto res = memo_convert_structure();
    auto parts = split(memo, ",");
    res.version = parts[0];
    auto path_elements = split(parts[1], " ");
    if (path_elements.size() == 1 && path_elements[0] == "") {
        res.path = {};
    }
    else
        res.path = split(parts[1], " ");

    res.min_return = parts[2];
    res.to_token = parts[3];
    return res;
}

memo_convert_structure next_hop(memo_convert_structure data){
    auto res = memo_convert_structure();
    res.path = std::vector<std::string>(data.path);
    res.path.erase(res.path.begin(), res.path.begin() + 2);
    res.version = data.version;
    res.min_return = data.min_return;
    res.to_token = data.to_token;
    return res;
}
