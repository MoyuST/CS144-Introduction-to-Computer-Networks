#include "router.hh"

#include <iostream>

using namespace std;

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    _routing_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
    
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    if(dgram.header().ttl<=1){
        return;
    }

    dgram.header().ttl--;

    int best_matched_rule_idx=-1;
    int best_matched_rule_prefix_size=-1;

    auto match_rule = [dgram](route_info r_info)->bool{
        uint32_t mask{0}, one_bit_mask{0x80000000};
        for(uint8_t i=r_info.prefix_length;i>0;i--){
            mask += (one_bit_mask>>(i-1));
        }

        return ((r_info.route_prefix&mask) == (dgram.header().dst&mask));
    };

    for(int i=0;i<static_cast<int>(_routing_table.size());i++){
        if(_routing_table[i].prefix_length>best_matched_rule_prefix_size){
            if(match_rule(_routing_table[i])){
                best_matched_rule_idx=i;
                best_matched_rule_prefix_size=_routing_table[i].prefix_length;
            }
        }
    }

    if(best_matched_rule_idx!=-1){
        if(_routing_table[best_matched_rule_idx].interface_num<_interfaces.size()){
            interface(_routing_table[best_matched_rule_idx].interface_num).send_datagram(dgram,
                _routing_table[best_matched_rule_idx].next_hop.has_value() ? 
                    _routing_table[best_matched_rule_idx].next_hop.value() : Address::from_ipv4_numeric(dgram.header().dst));
            return;
        }
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
