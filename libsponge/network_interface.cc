#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    EthernetFrame eth_frame{};
    eth_frame.header().src = _ethernet_address;
    eth_frame.header().type = EthernetHeader::TYPE_IPv4;
    eth_frame.payload() = dgram.serialize();

    if (_arp_cache.find(next_hop_ip) != _arp_cache.end()) {
        auto dst_eth = _arp_cache[next_hop_ip].first;
        eth_frame.header().dst = dst_eth;
        _frames_out.push(eth_frame);
    } else {
        if (_arp_in_flight.find(next_hop_ip) != _arp_in_flight.end() && _arp_in_flight[next_hop_ip] < 5000) {
            return;
        }

        ARPMessage arp_msg{};
        arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
        arp_msg.sender_ethernet_address = _ethernet_address;
        arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
        // arp_msg.target_ethernet_address = ETHERNET_BROADCAST;
        arp_msg.target_ip_address = next_hop_ip;
        EthernetFrame arp_eth_frame{};
        arp_eth_frame.header().src = _ethernet_address;
        arp_eth_frame.header().dst = ETHERNET_BROADCAST;
        arp_eth_frame.header().type = EthernetHeader::TYPE_ARP;
        arp_eth_frame.payload() = BufferList{Buffer{arp_msg.serialize()}};
        _frames_out.push(arp_eth_frame);
        _frames_out_dst_not_known.push_back({eth_frame, next_hop_ip});
        _arp_in_flight[next_hop_ip] = 0;
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage parse_msg{};
        if (!frame.payload().buffers().empty() &&
            parse_msg.parse(frame.payload().buffers().front()) == ParseResult::NoError) {
            _arp_cache[parse_msg.sender_ip_address] = {parse_msg.sender_ethernet_address, 30};
            auto it = _arp_in_flight.find(parse_msg.sender_ip_address);
            if (it != _arp_in_flight.end()) {
                _arp_in_flight.erase(it);
                for (auto it_not_send = _frames_out_dst_not_known.begin();
                     it_not_send != _frames_out_dst_not_known.end();) {
                    if (it_not_send->second == parse_msg.sender_ip_address) {
                        auto eth_frame = it_not_send->first;
                        eth_frame.header().dst = parse_msg.sender_ethernet_address;
                        _frames_out.push(eth_frame);
                        it_not_send = _frames_out_dst_not_known.erase(it_not_send);
                    } else {
                        it_not_send++;
                    }
                }
            }

            if (parse_msg.opcode == ARPMessage::OPCODE_REQUEST &&
                parse_msg.target_ip_address == _ip_address.ipv4_numeric() && frame.header().dst == ETHERNET_BROADCAST) {
                ARPMessage arp_msg{};
                arp_msg.opcode = ARPMessage::OPCODE_REPLY;
                arp_msg.sender_ethernet_address = _ethernet_address;
                arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
                arp_msg.target_ethernet_address = parse_msg.sender_ethernet_address;
                arp_msg.target_ip_address = parse_msg.sender_ip_address;
                EthernetFrame arp_eth_frame{};
                arp_eth_frame.header().src = _ethernet_address;
                arp_eth_frame.header().dst = parse_msg.sender_ethernet_address;
                arp_eth_frame.header().type = EthernetHeader::TYPE_ARP;
                arp_eth_frame.payload() = BufferList{Buffer{arp_msg.serialize()}};
                _frames_out.push(arp_eth_frame);
            }
        }
        return nullopt;
    }

    if (frame.header().dst != _ethernet_address) {
        return nullopt;
    }

    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram parse_msg{};
        if (!frame.payload().buffers().empty() &&
            parse_msg.parse(frame.payload().buffers().front()) == ParseResult::NoError) {
            return parse_msg;
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto it = _arp_cache.begin(); it != _arp_cache.end();) {
        if (it->second.second + ms_since_last_tick > 30000) {
            auto it_cp = it;
            it++;
            _arp_cache.erase(it_cp);
        } else {
            it->second.second += ms_since_last_tick;
            it++;
        }
    }

    for (auto it = _arp_in_flight.begin(); it != _arp_in_flight.end(); it++) {
        it->second += ms_since_last_tick;
    }
}
