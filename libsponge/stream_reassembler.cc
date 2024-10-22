#include "stream_reassembler.hh"

#include <algorithm>
#include <iostream>
#include <limits>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _buffer(capacity), _eof(numeric_limits<size_t>::max()) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (this->_output.input_ended()) {
        return;
    }
    if (eof) {
        this->_eof = min(index + data.size(), this->_eof);
    }

    size_t read_start = max(index, this->_start);
    size_t read_end =
        min(index + data.size(), min(this->_eof, this->_start + this->_capacity - this->_output.buffer_size()));

    for (size_t i = read_start; i < read_end; ++i) {
        auto &cur = this->_buffer[i % this->_capacity];
        if (cur.second) {
            continue;
        }
        this->_num_unassembled++;
        cur.first = data[i - index];
        cur.second = true;
    }

    while (this->_start < this->_eof && this->_buffer[this->_start % this->_capacity].second) {
        auto &cur = this->_buffer[this->_start % this->_capacity];
        this->_output.write(string(1, cur.first));
        cur.second = false;
        this->_start++;
        this->_num_unassembled--;
    }

    if (this->_start == this->_eof) {
        this->_output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return this->_num_unassembled; }

bool StreamReassembler::empty() const { return this->_num_unassembled == 0; }
