#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

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
    auto it = _cache.find(next_hop_ip);

    if(it != _cache.end()){ //if DES MAC already in cache
        //send the ethernet frame directly
        _frames_out.emplace(encapsulate((*it).second.first, EthernetHeader::TYPE_IPv4, dgram.serialize()));
    }else{
        //DES MAC not in cache
        //broadcast ARP request if currently we aren't requesting it
        if(_arp_waiting.find(next_hop_ip) == _arp_waiting.end()){
            ARPMessage arp_request;
            arp_request.opcode = ARPMessage :: OPCODE_REQUEST;
            arp_request.sender_ethernet_address = _ethernet_address;
            arp_request.target_ethernet_address = {};
            arp_request.sender_ip_address = _ip_address.ipv4_numeric();
            arp_request.target_ip_address = next_hop_ip;
            //send ARP request
            _frames_out.emplace(encapsulate(ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, arp_request.serialize()));
            //add reqeust to waiting
            _arp_waiting[next_hop_ip] = ARP_RESPONSE_TTL;
        }
        //add IP packet to waiting queue
        _packets_waiting.emplace_back(make_pair(next_hop_ip, dgram));
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    //if the frame is not a broad cast and not for us, ignore it and return
    if (frame.header().dst != ETHERNET_BROADCAST && frame.header().dst != _ethernet_address) {
        return nullopt;
    }
    //if it is an IPv4 frame, parse and return it
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram ret;
        if (ret.parse(frame.payload()) == ParseResult::NoError){
            return ret;
        }
        return nullopt;
    }else{
        //not an IPv4, then must be an ARP datagram
        ARPMessage arp_msg;
        if (arp_msg.parse(frame.payload()) != ParseResult::NoError){
            return nullopt;
        }
        const uint32_t src_ip = arp_msg.sender_ip_address;
        //if this is an ARP request for me
        if(arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == _ip_address.ipv4_numeric()){
            //generate a ARP reply
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = _ethernet_address;
            arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
            arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
            arp_reply.target_ip_address = src_ip;
            //send the ARP reply
            _frames_out.emplace(encapsulate(arp_msg.sender_ethernet_address, EthernetHeader::TYPE_ARP, arp_reply.serialize()));
        }

        //even the ARP request is not for us, still leaern the mapping and cache it
        _cache[src_ip] = {arp_msg.sender_ethernet_address, ARP_ENTRY_TTL};

        //since we learned a mapping, check if there is any packets that we can send now
        auto it = _packets_waiting.begin();
        //check waiting queue and send all packets whose DES IP is this newly leared IP mapping
        while (it != _packets_waiting.end()) {
            if (it->first == src_ip) {
                _frames_out.emplace(encapsulate(arp_msg.sender_ethernet_address, EthernetHeader::TYPE_IPv4, it->second.serialize()));
                it = _packets_waiting.erase(it);
            } else {
                ++it;
            }
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    //remove expired entry in cache
    for (auto it = _cache.begin(); it != _cache.end();) {
        if (it->second.second <= ms_since_last_tick) {
            it = _cache.erase(it);
        } else {
            it->second.second -= ms_since_last_tick;
            ++it;
        }
    }

    for (auto it = _arp_waiting.begin(); it != _arp_waiting.end();) {
        if (it->second <= ms_since_last_tick) {
            it = _arp_waiting.erase(it);
        } else {
            it->second -= ms_since_last_tick;
            ++it;
        }
    }
}

EthernetFrame NetworkInterface::encapsulate(const EthernetAddress &dst, uint16_t type, const BufferList &payload){
    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().dst = dst;
    frame.header().type = type;
    frame.payload() = payload;
    return frame;
}
