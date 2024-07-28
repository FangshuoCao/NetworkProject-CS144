#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _buf(), _next(0),
_eof(numeric_limits<uint64_t>::max()), _unassembled_bytes(0), _output(capacity),
_capacity(capacity){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _next + _capacity) {
        return;
    }

    uint64_t end_index = min(index + data.size(), 
    min(_next + _capacity - _output.buffer_size(), _eof));

    if (eof) {
        _eof = min(_eof, index + data.size());
    }

    for (uint64_t i = index; i < end_index; ++i) {
        if (i < _next) {
            continue;
        }

        if (_buf.find(i) == _buf.end()) {
            _buf[i] = data[i - index];
            ++_unassembled_bytes;
        }
    }

    string str;
    while (_buf.find(_next) != _buf.end()) {
        str += _buf[_next];
        _buf.erase(_next);
        ++_next;
        --_unassembled_bytes;
    }

    _output.write(str);

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
