#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _buffer(capacity, '\0'), _received(capacity, false), _next(0),
      _eof(std::numeric_limits<size_t>::max()), _unassembled_bytes(0),
      _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _next + _capacity) {
        return;
    }

    size_t start_index = std::max(index, _next);
    size_t end_index = std::min(index + data.size(), min(_eof, _next + _capacity - _output.buffer_size()));

    if (eof) {
        _eof = std::min(_eof, index + data.size());
    }

    for (size_t i = start_index; i < end_index; ++i) {
        if (!_received[i - _next]) {
            _buffer[i - _next] = data[i - index];
            _received[i - _next] = true;
            ++_unassembled_bytes;
        }
    }

    std::string str;
    while (_next < _capacity && _received[_next]) {
        str += _buffer[_next];
        _received[_next] = false; // Mark as processed
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
