#include "XTransferRerouter.hpp"

using namespace eosio;

ACTION XTransferRerouter::enablerrt(bool enable) {
    require_auth(get_self());

    settings settings_table(get_self(), get_self().value);
    settings_table.set(settings_t{enable}, get_self());
}

ACTION XTransferRerouter::reroutetx(uint64_t tx_id, string blockchain, string target) {
    settings settings_table(get_self(), get_self().value);
    auto st = settings_table.get();
    
    check(st.rrt_enabled, "transaction rerouting is disabled");

    EMIT_TX_REROUTE_EVENT(tx_id, blockchain, target);

}

EOSIO_DISPATCH(XTransferRerouter, (enablerrt)(reroutetx))