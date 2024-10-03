#include "timer.hh"

void Timer::track(TCPSegment &seg) {
    this->_outstandings.push(pair(this->_nx_seqno, seg));
    this->_nx_seqno += seg.length_in_sequence_space();
    this->_running = true;
}

bool Timer::remove_received(const WrappingInt32 ackno) {
    uint64_t abs_ackno = unwrap(ackno, this->_isn, this->_nx_seqno);

    // Invalid ACK
    if (abs_ackno > this->_nx_seqno) {
        return false;
    }

    uint64_t ack_len = 0;
    while (!this->_outstandings.empty()) {
        auto outstand = this->_outstandings.front();

        if (abs_ackno < outstand.first + outstand.second.length_in_sequence_space()) {
            break;
        }

        ack_len += outstand.second.length_in_sequence_space();
        this->_outstandings.pop();
        this->_num_retransmissions = 0;
        this->_rto = this->_initial_retransmission_timeout;
        this->_time_elapsed = 0;
    }

    if (this->_outstandings.empty()) {
        this->_running = false;
    }

    this->_num_acked += ack_len;
    return true;
}

bool Timer::resend(const size_t ms_since_last_tick) {
    if (!this->_running) {
        return false;
    }

    this->_time_elapsed += ms_since_last_tick;
    if (this->_time_elapsed < this->_rto || this->_outstandings.empty()) {
        return false;
    }

    this->_time_elapsed = 0;
    return true;
}