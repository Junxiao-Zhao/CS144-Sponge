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

    EthernetFrame frame;
    frame.header().src = this->_ethernet_address;

    // destination already known
    if (this->_arp_table.find(next_hop_ip) != this->_arp_table.end()) {
        frame.header().dst = this->_arp_table[next_hop_ip].first;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.payload() = dgram.serialize();
        this->_frames_out.emplace(move(frame));
        return;
    }

    // send a new ARP request
    if (this->_arp_request_time.find(next_hop_ip) == this->_arp_request_time.end()) {
        ARPMessage arp_request;
        arp_request.opcode = ARPMessage::OPCODE_REQUEST;
        arp_request.sender_ethernet_address = this->_ethernet_address;
        arp_request.sender_ip_address = this->_ip_address.ipv4_numeric();
        arp_request.target_ip_address = next_hop_ip;

        frame.header().dst = ETHERNET_BROADCAST;
        frame.header().type = EthernetHeader::TYPE_ARP;
        frame.payload() = arp_request.serialize();
        this->_frames_out.emplace(move(frame));
        this->_arp_request_time[next_hop_ip] = this->_arp_request_interval;
    }

    this->_pending_datagrams[next_hop_ip].emplace_back(dgram, next_hop);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    auto frame_header = frame.header();
    if (frame_header.dst != ETHERNET_BROADCAST && frame_header.dst != this->_ethernet_address) {
        return {};
    }
    
    if (frame_header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
        return {};
    }

    if (frame_header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_request;
        if (arp_request.parse(frame.payload()) != ParseResult::NoError) {
            return {};
        }

        if (arp_request.opcode == ARPMessage::OPCODE_REQUEST && arp_request.target_ip_address == this->_ip_address.ipv4_numeric()) {
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = this->_ethernet_address;
            arp_reply.sender_ip_address = this->_ip_address.ipv4_numeric();
            arp_reply.target_ethernet_address = arp_request.sender_ethernet_address;
            arp_reply.target_ip_address = arp_request.sender_ip_address;

            EthernetFrame fgram;
            fgram.header().src = this->_ethernet_address;
            fgram.header().dst = arp_request.sender_ethernet_address;
            fgram.header().type = EthernetHeader::TYPE_ARP;
            fgram.payload() = arp_reply.serialize();

            this->_frames_out.emplace(move(fgram));
        }

        this->_arp_table[arp_request.sender_ip_address] = {arp_request.sender_ethernet_address, this->_arp_request_timeout};

        if (this->_pending_datagrams.find(arp_request.sender_ip_address) != this->_pending_datagrams.end()) {
            for (auto &dgram : this->_pending_datagrams[arp_request.sender_ip_address]) {
                this->send_datagram(dgram.first, dgram.second);
            }
            this->_pending_datagrams.erase(arp_request.sender_ip_address);
        }
    }

    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto it = this->_arp_table.begin(); it != this->_arp_table.end();) {
        if (it->second.second <= ms_since_last_tick) {
            it = this->_arp_table.erase(it);
        } else {
            it->second.second -= ms_since_last_tick;
            ++it;
        }
    }

    for (auto it = this->_arp_request_time.begin(); it != this->_arp_request_time.end();) {
        if (it->second <= ms_since_last_tick) {
            if (this->_pending_datagrams.find(it->first) != this->_pending_datagrams.end()) {
                 this->_pending_datagrams.erase(it->first);
            }
            it = this->_arp_request_time.erase(it);
        } else {
            it->second -= ms_since_last_tick;
            ++it;
        }
    }
}
