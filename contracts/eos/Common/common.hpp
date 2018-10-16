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


struct memoConvertStructure {
    path    path;
    std::string   version;
    std::string   target;
    std::string   min_return;
};
    
std::string buildMemo(memoConvertStructure data){
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
    memo.append(data.target);
    return memo;
}

memoConvertStructure parseMemo(std::string memo){
    auto res = memoConvertStructure();
    auto parts = split(memo, ",");
    res.version = parts[0];
    auto pathElements = split(parts[1], " ");
    if(pathElements.size() == 1 && pathElements[0] == ""){
        res.path = {};
    }
    else
        res.path = split(parts[1], " ");
    res.min_return = parts[2];
    res.target = parts[3];
    return res;
}

memoConvertStructure nextHop(memoConvertStructure data){
    auto res = memoConvertStructure();
    res.path = std::vector<std::string>(data.path);
    res.path.erase(res.path.begin(), res.path.begin() + 3);
    res.version = data.version;
    res.min_return = data.min_return;
    res.target = data.target;
    return res;
}