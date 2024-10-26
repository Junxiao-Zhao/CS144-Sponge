#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "timer.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return this->_next_seqno - this->_timer.num_acked(); }

void TCPSender::fill_window() {
    uint16_t window_size = max(this->_window_size, static_cast<uint16_t>(1));

    while (window_size > bytes_in_flight()) {
        TCPSegment seg;

        // CLOSED -> SYN_SENT
        if (!this->_syn_sent) {
            seg.header().syn = true;
            this->_syn_sent = true;
        }

        uint16_t payload_size =
            min(TCPConfig::MAX_PAYLOAD_SIZE,
                min(window_size - bytes_in_flight() - seg.header().syn, this->_stream.buffer_size()));
        auto payload_content = this->_stream.read(payload_size);
        seg.payload() = Buffer(move(payload_content));

        // FIN_SENT
        if (!this->_fin_sent && this->_stream.eof() &&
            window_size > bytes_in_flight() + seg.length_in_sequence_space()) {
            seg.header().fin = true;
            this->_fin_sent = true;
        }

        uint64_t length = seg.length_in_sequence_space();

        // Throw away empty segments
        if (length == 0) {
            break;
        }

        seg.header().seqno = next_seqno();
        this->_segments_out.push(seg);
        this->_next_seqno += length;
        this->_timer.track(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if (this->_timer.remove_received(ackno)) {
        this->_window_size = window_size;
        this->fill_window();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // Not yet timeout OR no outstanding segments
    if (!this->_timer.resend(ms_since_last_tick)) {
        return;
    }

    this->_segments_out.push(this->_timer.outstandings().front().second);
    if (this->_window_size > 0) {
        this->_timer.extend_rto();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return this->_timer.num_retransmissions(); }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    this->_segments_out.push(seg);
}
