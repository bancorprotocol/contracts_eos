
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */

#include "../Common/common.hpp"
#include "../Token/Token.hpp"
#include "BancorNetwork.hpp"

ACTION BancorNetwork::setmaxfee(uint64_t max_affiliate_fee) {
    require_auth(get_self());

    settings settings_table(get_self(), get_self().value);
    auto st = settings_table.find("settings"_n.value);
    check(max_affiliate_fee > 0 && max_affiliate_fee <= MAX_FEE, "fee outside resolution");

    if (st == settings_table.end())
        settings_table.emplace(get_self(), [&](auto& s) {		
            s.enabled = true;
            s.max_fee = max_affiliate_fee;
        });
    else
        settings_table.modify(st, same_payer, [&](auto& s) {		
            s.max_fee = max_affiliate_fee;
        });
}

void BancorNetwork::on_transfer(name from, name to, asset quantity, string memo) {
    // avoid unstaking and system contract ops mishaps
    if (from == get_self() || from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) 
        return;

    check(quantity.symbol.is_valid(), "invalid quantity in transfer");
    check(quantity.amount > 0, "may only transfer positive quantity");
    
    settings settings_table(get_self(), get_self().value);
    auto st = settings_table.find("settings"_n.value);
    check(st != settings_table.end(), "create network settings");

    auto memo_object = parse_memo(memo); 
    asset new_quantity = quantity;

    if (!memo_object.path.size()) { // just exited from the last conversion in the path
        tie(new_quantity, memo_object) = pay_affiliate(from, quantity, st->max_fee, memo_object);
        to = name(memo_object.dest_account.c_str());
        string trader = memo_object.trader_account;
        memo = memo_object.receiver_memo;

        check(!trader.empty() && is_account(name(trader.c_str())), "invalid memo");
        verify_entry(to, get_first_receiver(), quantity.symbol);
        verify_min_return(new_quantity, memo_object.min_return);
    } else {
        auto path_size = memo_object.path.size();
        to = memo_object.converters[0].account;
        
        check(path_size >= 2 && !(path_size % 2), "bad path format");
        check(isConverter(to), "converter doesn't exist");

        if (memo_object.trader_account.empty()) { // about to enter the first conversion in the path
            memo_object.trader_account = from.to_string();
            memo = build_memo(memo_object);
        } else 
            tie(new_quantity, memo_object) = pay_affiliate(from, quantity, st->max_fee, memo_object);
    }
    if (new_quantity.amount != quantity.amount) // if affiliate fee was deducted from was from quantity
        memo = build_memo(memo_object);
        
    action(
        permission_level{ get_self(), "active"_n },
        get_first_receiver(), "transfer"_n,
        make_tuple(get_self(), to, new_quantity, memo)
    ).send();
}

tuple<asset, memo_structure> BancorNetwork::pay_affiliate(name from, asset quantity, uint64_t max_fee, memo_structure memo) {
    if (quantity.symbol.code() == symbol_code("BNT") && !memo.affiliate_account.empty()) {
        name affiliate = name(memo.affiliate_account);
        uint64_t fee = stoui(memo.affiliate_fee);
        
        check(get_first_receiver() == BNT_TOKEN, "BNT quantity received is not authentic");
        check(fee > 0 && max_fee > fee, "inappropriate affiliate fee");
        check(is_account(affiliate), "affiliate is not an account");    

        double amount = quantity.amount / pow(10, quantity.symbol.precision());
        uint64_t fee_amount = calculate_fee(amount, fee, 1) * pow(10, quantity.symbol.precision());
        
        if (fee_amount > 0) {
            asset affiliate_fee = asset(fee_amount, quantity.symbol);
            action(
                permission_level{ get_self(), "active"_n },
                BNT_TOKEN, "transfer"_n,
                make_tuple(get_self(), affiliate, affiliate_fee, string("affiliate pay"))
            ).send();
            EMIT_AFFILIATE_FEE_EVENT(memo.trader_account, from, affiliate, quantity, affiliate_fee);

            string quantity_before = quantity.to_string();
            quantity -= affiliate_fee;
            string quantity_after = quantity.to_string();
        }
        memo.affiliate_account = "";
        memo.affiliate_fee = "";
    }
    return make_tuple(quantity, memo);
}

bool BancorNetwork::isConverter(name converter) {
    settings settings_table(converter, converter.value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    return st.enabled;
}

// asserts if a conversion resulted in an amount lower than the minimum amount defined by the caller
void BancorNetwork::verify_min_return(asset quantity, string min_return) {
	float ret = stof(min_return.c_str());
    uint64_t ret_amount = ret * pow(10, quantity.symbol.precision());
    if (ret_amount)
        check(quantity.amount >= ret_amount, "below min return");
    else
        check(quantity.amount > 0, "return must be above zero");
}

// asserts if the supplied account doesn't have an entry for a given token
void BancorNetwork::verify_entry(name account, name currency_contract, symbol currency) {
    check(is_account(account), "destination is not an account");
    Token::accounts accountstable(currency_contract, account.value);
    auto ac = accountstable.find(currency.code().raw());
    check(ac != accountstable.end(), "must have entry for token (claim token first)");
}
