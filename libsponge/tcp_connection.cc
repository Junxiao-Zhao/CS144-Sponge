#include "tcp_connection.hh"

#include <iostream>
#include <limits>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return this->_sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return this->_sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return this->_receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return this->_time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    this->_time_since_last_segment_received = 0;

    if (seg.header().rst) {  // Error
        set_rst();
        return;
    }

    this->_receiver.segment_received(seg);

    bool need_send_ack = seg.length_in_sequence_space() > 0;

    if (seg.header().ack) {
        this->_sender.ack_received(seg.header().ackno, seg.header().win);
        if (need_send_ack && !this->_sender.segments_out().empty())
            need_send_ack = false;
    }

    if (TCPState::state_summary(this->_receiver) == TCPReceiverStateSummary::SYN_RECV &&
        TCPState::state_summary(this->_sender) == TCPSenderStateSummary::CLOSED) {
        connect();
        return;
    }

    // ask to passive close; no need to linger
    if (TCPState::state_summary(this->_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(this->_sender) == TCPSenderStateSummary::SYN_ACKED) {
        this->_linger_after_streams_finish = false;
    }

    // Passive close
    if (!this->_linger_after_streams_finish &&
        TCPState::state_summary(this->_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(this->_sender) == TCPSenderStateSummary::FIN_ACKED) {
        this->_active = false;
        return;
    }

    // Keep alive
    if (this->_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
        seg.header().seqno == this->_receiver.ackno().value() - 1) {
        need_send_ack = true;
    }

    if (need_send_ack) {
        this->_sender.send_empty_segment();
    }

    send_segments();
}

bool TCPConnection::active() const { return this->_active; }

size_t TCPConnection::write(const string &data) {
    size_t write_size = this->_sender.stream_in().write(data);
    this->_sender.fill_window();
    send_segments();
    return write_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    this->_sender.tick(ms_since_last_tick);
    this->_time_since_last_segment_received += ms_since_last_tick;

    if (this->_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        queue<TCPSegment> empty;
        swap(this->_sender.segments_out(), empty);
        send_rst();
        set_rst();
        return;
    }

    send_segments();

    // Active close
    if (this->_linger_after_streams_finish &&
        TCPState::state_summary(this->_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(this->_sender) == TCPSenderStateSummary::FIN_ACKED &&
        this->_time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
        this->_active = false;
        this->_linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    this->_sender.stream_in().end_input();
    this->_sender.fill_window();
    send_segments();
}

void TCPConnection::connect() {
    this->_sender.fill_window();
    send_segments();
}

void TCPConnection::send_segments() {
    while (!this->_sender.segments_out().empty()) {
        auto seg = move(this->_sender.segments_out().front());
        this->_sender.segments_out().pop();

        if (this->_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = this->_receiver.ackno().value();
        }
        seg.header().win = min(static_cast<size_t>(numeric_limits<uint16_t>::max()), this->_receiver.window_size());
        this->_segments_out.emplace(move(seg));
    }
}

void TCPConnection::set_rst() {
    this->_sender.stream_in().set_error();
    this->_receiver.stream_out().set_error();
    this->_active = false;
    this->_linger_after_streams_finish = false;
}

void TCPConnection::send_rst() {
    TCPSegment seg;
    seg.header().rst = true;
    seg.header().seqno = this->_sender.next_seqno();
    this->_segments_out.emplace(move(seg));
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_rst();
            set_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
