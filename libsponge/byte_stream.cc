#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//constructor:
//initialize the buffer to size capacity+1 since it is circular,
//in order to distinguish between full buf and empty buf
ByteStream::ByteStream(const size_t capacity)
    :_buf(capacity + 1), _capacity(capacity), _cnt_written(0),
    _cnt_read(0), _head(0), _tail(0) {}

size_t ByteStream::write(const string &data) {
    if(_input_ended) return 0;
    //if data is larger than remaining capacity, write untill stream is full
    size_t written_len = min(data.size(), remaining_capacity());
    for(size_t i = 0; i < written_len; i++){
        _buf[_tail] = data[i];  //write to tail
        //use % to handle update in circular vector
        _tail = (_tail + 1) % (+_capacity + 1);
    }
    _cnt_written += written_len;
    return written_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string str;
    size_t peeked_len = min(len, buffer_size());
    for(size_t i = 0; i < peeked_len; i++){
        //again, use % to handle circular vector
        str.append(1, _buf[(_head + i) % (_capacity + 1)]);
    }
    return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t popped_len = min(len, buffer_size());
    _head = (_head + popped_len) % (_capacity + 1);
    _cnt_read += popped_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string str = peek_output(len);  //read
    pop_output(len);    //then pop
    return str;
}

void ByteStream::end_input() {
     _input_ended = true;
}

bool ByteStream::input_ended() const {
    return _input_ended;
}

size_t ByteStream::buffer_size() const {
    if (_head <= _tail) {
        return _tail - _head;
    } else {
        return (_tail + _capacity + 1) - _head;
    }
}

bool ByteStream::buffer_empty() const {
    return buffer_size() == 0;
}

bool ByteStream::eof() const {
    return _input_ended && buffer_empty();
}

size_t ByteStream::bytes_written() const {
    return _cnt_written;
}

size_t ByteStream::bytes_read() const {
    return _cnt_read;
}

size_t ByteStream::remaining_capacity() const {
    return _capacity - buffer_size();
}
