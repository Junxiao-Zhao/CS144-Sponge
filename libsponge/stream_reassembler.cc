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
    : _output(capacity), _capacity(capacity), _buffer(), _eof(numeric_limits<size_t>::max()) {}

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

    for (size_t i = 0; i < data.size(); ++i) {
        if (i + index < this->_start) {
            continue;
        }

        auto exist = this->_buffer.find(i + index);
        if (exist == this->_buffer.end()) {
            this->_num_unassembled++;
        }
        this->_buffer[i + index] = data[i];
    }

    auto iter = this->_buffer.find(this->_start);

    while (this->_start < this->_eof && this->_output.remaining_capacity() > 0 && iter != this->_buffer.end()) {
        this->_output.write(string(1, iter->second));
        cout << "write <" << string(1, iter->second) << "> at: " << this->_start << endl;
        cout << "remain capa2: " << this->_output.remaining_capacity() << endl;
        --this->_num_unassembled;
        iter = this->_buffer.find(++this->_start);
    }

    if (this->_start == this->_eof) {
        this->_output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return this->_num_unassembled; }

bool StreamReassembler::empty() const { return this->_num_unassembled == 0; }
