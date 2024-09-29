#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity), buffer(capacity, '\0') {}

size_t ByteStream::write(const string &data) {
    size_t remain_capa = this->remaining_capacity();
    if (this->ended || remain_capa == 0) {
        return 0;
    }

    size_t copy_size = min(remain_capa, data.size());
    size_t suffix_size = this->_capacity - this->end;
    if (this->end < this->start || suffix_size >= copy_size) {
        copy_n(data.begin(), copy_size, this->buffer.begin() + this->end);
    } else {
        copy_n(data.begin(), suffix_size, this->buffer.begin() + this->end);
        copy_n(data.begin() + suffix_size, copy_size - suffix_size, this->buffer.begin());
    }
    this->end += copy_size;
    this->end %= this->_capacity;
    this->cur_size += copy_size;
    this->write_cnt += copy_size;
    return copy_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string output;
    size_t peek_len = min(len, this->cur_size);
    if (peek_len == 0) {
        return output;
    }
    // "<" instead of "<=" since we could not tell if the buffer is full or empty
    if (start < end || start + peek_len <= this->_capacity) {
        output = string{this->buffer.begin() + this->start, this->buffer.begin() + this->start + peek_len};
    } else {
        output.append(string{this->buffer.begin() + this->start, this->buffer.end()} +
                      string{this->buffer.begin(), this->buffer.begin() + peek_len - (this->_capacity - this->start)});
    }
    return output;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t peek_len = min(len, this->cur_size);
    this->start += peek_len;
    this->start %= this->_capacity;
    this->cur_size -= peek_len;
    this->read_cnt += peek_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string output = this->peek_output(len);
    this->pop_output(len);
    return output;
}

void ByteStream::end_input() { this->ended = true; }

bool ByteStream::input_ended() const { return this->ended; }

size_t ByteStream::buffer_size() const { return this->cur_size; }

bool ByteStream::buffer_empty() const { return this->cur_size == 0; }

bool ByteStream::eof() const { return this->input_ended() && this->buffer_empty(); }

size_t ByteStream::bytes_written() const { return this->write_cnt; }

size_t ByteStream::bytes_read() const { return this->read_cnt; }

size_t ByteStream::remaining_capacity() const { return this->_capacity - this->cur_size; }
