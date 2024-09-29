#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();

    if (header.syn) {
        this->_isn = header.seqno;
        this->_isn_received = true;
    }

    // throw away segments received before ISN
    if (!this->_isn_received) {
        return;
    }

    uint64_t abs_ackno = this->_reassembler.stream_out().bytes_written() + 1;
    uint64_t abs_seqno = unwrap(header.seqno, this->_isn, abs_ackno);
    uint64_t _index = abs_seqno - 1 + header.syn;

    Buffer payload = seg.payload();
    this->_reassembler.push_substring(payload.copy(), _index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (this->_isn_received) {
        uint64_t abs_ackno = this->_reassembler.stream_out().bytes_written() + 1;
        if (this->_reassembler.stream_out().input_ended()) {
            abs_ackno++;
        }
        return wrap(abs_ackno, this->_isn);
    }
    return {};
}

size_t TCPReceiver::window_size() const { return this->_capacity - this->_reassembler.stream_out().buffer_size(); }
