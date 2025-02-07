#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

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

    // Your code here.
    this->_routes.emplace_back(move(RouterRow{route_prefix, prefix_length, next_hop, interface_num}));
}

RouterRow* Router::longest_prefix_match(const uint32_t dest_ip) {
    RouterRow *longest_match = nullptr;
    size_t longest_match_length = 0;
    size_t ip_prefix = this->MASK;
    for (auto &router : this->_routes) {
        ip_prefix = dest_ip & (this->MASK << (32 - router.prefix_length));

        if (ip_prefix == router.route_prefix && router.prefix_length >= longest_match_length) {
            longest_match = &router;
            longest_match_length = router.prefix_length;
        }
    }

    return longest_match;
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    uint32_t dest_ip = dgram.header().dst;
    RouterRow *longest_match = this->longest_prefix_match(dest_ip);

    if (longest_match == nullptr || dgram.header().ttl <= 1) {
        return;
    }

    dgram.header().ttl -= 1;
    dest_ip = longest_match->next_hop.has_value() ? longest_match->next_hop->ipv4_numeric() : dest_ip;
    this->_interfaces[longest_match->interface_num].send_datagram(dgram, Address::from_ipv4_numeric(dest_ip));
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
