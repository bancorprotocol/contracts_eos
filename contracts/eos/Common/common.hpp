
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
    string            sender_account;
    string            affiliate_account;
    string	          affiliate_fee;
    string            receiver_memo;
};

#define BANCOR_X "bancorxoneos"_n
#define BANCOR_NETWORK "thisisbancor"_n
#define BNT_TOKEN "bntbntbntbnt"_n

constexpr static double MAX_RATIO = 1000000.0;
constexpr static double MAX_FEE = 1000000.0;

vector<string> split(const string& str, const string& delim) {
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    return tokens;
}

string build_memo(memo_structure data) {
    string pathstr = "";
    for (auto i = 0; i < data.path.size(); i++) {
        if (i != 0) pathstr.append(" ");
        pathstr.append(data.path[i]);
    }
    string memo = "";
    memo.append(data.version);
    memo.append(",");
    memo.append(pathstr);
    memo.append(",");
    memo.append(data.min_return);
    memo.append(",");
    memo.append(data.dest_account);
    if (!data.sender_account.empty()) {
        memo.append(",");
        memo.append(data.sender_account);
    }
    if (!data.affiliate_account.empty()) {
        memo.append(",");
        memo.append(data.affiliate_account);
        memo.append(",");
        memo.append(data.affiliate_fee);
    }
    memo.append(";");
    memo.append(data.receiver_memo);

    return memo;
}

/** @dev to_fixed 
 *  formats a number to a fixed precision
 *  e.g. - to_fixed(14.214212, 3) --> 14.214
*/
double to_fixed(double num, int precision) {
    return (int)(num * pow(10.0, precision)) / pow(10.0, precision);
}

float stof(const char* s) {
    float rez = 0, fact = 1;
    
    if (*s == '-') s++; //skip the sign
    for (int point_seen = 0; *s; s++) {
        if (*s == '.') {
            if (point_seen) return 0;
            point_seen = 1; 
            continue;
        }
        int d = *s - '0';
        if (d >= 0 && d <= 9) {
            if (point_seen) fact /= 10.0f;
            rez = rez * 10.0f + (float)d;
        } else return 0;
    }
    return rez * fact;
}

double calculate_fee(double amount, uint64_t fee, uint8_t magnitude) {
    return amount * (1 - pow((1 - fee / MAX_FEE), magnitude));
}

uint64_t stoui(string const& value) {
  uint64_t result = 0;
  size_t const length = value.size();
  switch (length) {
    case 7: result += (value[length -  7] - '0') * 1000000ULL;
    case 6: result += (value[length -  6] - '0') * 100000ULL;
    case 5: result += (value[length -  5] - '0') * 10000ULL;
    case 4: result += (value[length -  4] - '0') * 1000ULL;
    case 3: result += (value[length -  3] - '0') * 100ULL;
    case 2: result += (value[length -  2] - '0') * 10ULL;
    case 1: result += (value[length -  1] - '0');
  }
  return result;
}

memo_structure parse_memo(string memo) {
    memo_structure res = memo_structure();
    
    vector<string> split_memos = split(memo, ";"); // we separate concantenated memos with ";"
    vector<string> parts = split(split_memos[0], ","); // split the first memo by ","
    
    check(parts.size() >= 4 && parts.size() <= 7, "invalid memo");
    
    res.converters = {};
    res.version = parts[0];

    auto path_elements = split(parts[1], " ");

    if (path_elements.size() == 1 && path_elements[0] == "")
        res.path = {};
    else
        res.path = path_elements;
    
    for (int i = 0; i < res.path.size(); i += 2) {
        auto converter_data = split(res.path[i], ":");
        auto cnvrt = converter();
        cnvrt.account = name(converter_data[0].c_str());
        cnvrt.sym = converter_data.size() > 1 ? converter_data[1] : "";
        res.converters.push_back(cnvrt);
    }
    if (split_memos.size() == 2)
        res.receiver_memo = split_memos[1];
    else
        res.receiver_memo = "convert"; // default memo for receiver account

    res.min_return = parts[2];
    res.dest_account = parts[3];
    
    // supplying an affiliate account without affiliate fee 
    // will interpret ^account as sender of the conversion
    if (parts.size() == 5) { // or no affiliate parts at all
        res.sender_account = parts[4];
    }
    // affiliate parts present, but sender not yet set
    else if (parts.size() == 6) { 
        res.affiliate_account = parts[4];
        res.affiliate_fee = parts[5];
    }
    // affiliate parts present, AND sender already set
    else if (parts.size() == 7) {
        res.sender_account = parts[4];
        res.affiliate_account = parts[5];
        res.affiliate_fee = parts[6];
    }
    return res;
}
