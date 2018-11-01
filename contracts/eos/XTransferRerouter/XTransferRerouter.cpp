#include "./XTransferRerouter.hpp"

using namespace eosio;

ACTION XTransferRerouter::enablerrt(bool enable) {
    require_auth(_self);

    settings settings_table(_self, _self.value);
    settings_table.set(settings_t{ 
        enable
        }, _self);
}

ACTION XTransferRerouter::reroutetx(uint64_t tx_id, string blockchain, string target) {
    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    
    eosio_assert(st.rrt_enabled, "transaction rerouting is disabled");

    EMIT_TX_REROUTE_EVENT(tx_id, blockchain, target);

}

EOSIO_DISPATCH(XTransferRerouter, (enablerrt)(reroutetx))