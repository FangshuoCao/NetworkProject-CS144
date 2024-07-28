#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _buf(), _next(0),
_eof(numeric_limits<size_t>::max()), _unassembled_bytes(0), _output(capacity),
_capacity(capacity){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _next + _capacity) {
        return;
    }

    //handle overlap, if data overlap with bytes in the output stream(index < _next),
    //we should start at next
    size_t start_index = max(index, _next);

    //handles truncation: the last byte to be read is the smallest among three values:
    //the end index of data;
    //the capacity left in the buffer, which is
    //next expected index + total capacity - bytes in the output buffer that has not been read;
    //and EOF;
    size_t end_index = 
    min(index + data.size(),min(_eof, _next + _capacity - _output.buffer_size())); 

    //if eof is set, update _eof to the last byte of data
    //note that in our constructor, we initialize eof to infinity
    if (eof) {
        _eof = min(_eof, index + data.size());
    }

    //copy data from start index to end index into buffer
    for (size_t i = start_index; i < end_index; ++i) {
        //only copy if index is not present in map
        if (_buf.find(i) == _buf.end()) {
            _buf[i] = data[i - index]; //char in data still start at 0
            ++_unassembled_bytes;
        }
    }

    //write from buffer to output
    string str;
    while (_buf.find(_next) != _buf.end()) {
        str += _buf[_next];
        _buf.erase(_next);
        ++_next;
        --_unassembled_bytes;
    }
    _output.write(str);

    //if we reach eof, end input
    if (_next == _eof) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    return _unassembled_bytes; 
}

bool StreamReassembler::empty() const {
    return _unassembled_bytes == 0;
}
