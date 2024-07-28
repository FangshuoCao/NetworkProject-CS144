#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
_output(capacity), _capacity(capacity), _next(0), _unassembled_bytes(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //start index of substring completely out of bound of the buffer
    if(index > _next + _capacity){
        return;
    }

    //handle overlapping substr
    //adjust the starting index in the substr data if overlapping
    size_t start_in_data = (index < _next) ? _next - index : 0;
    //adjust the starting index to insert data into output
    size_t start_in_output = (index < _next) ? _next : index;
    //if the entire substring is already in the output, don't need to do anything
    if(start_in_data >= data.size()){
        return;
    }
    
    //handle truncation
    //the actual substr of data we need to insert, after adjusting for overlap
    string str_to_insert = data.substr(start_in_data);
    uint64_t end_index = start_in_output + str_to_insert.size();
    if(end_index > _next + _capacity){ //out of bound, need truncation
        str_to_insert = str_to_insert.substr(0, _next + _capacity - start_in_output);
    }

    //now we finish modifying the input
    //str_to_insert is the final string we need to insert at position _start_in_output
    
    if(start_in_output == _next){   //string to push matches the expected next index
        //then directly write to output
        _next += _output.write(str_to_insert);

        //check if there is any buffered substring that can now be written to
        //ouput after a new write
        while(!_buf.empty() && _buf.begin()->first == _next){
            _next += _output.write(_buf.begin()->second);
            _unassembled_bytes -= _buf.begin()->second.size();
            _buf.erase(_buf.begin());
        }
    }else{  //index of string to push does not match
        //put it into the buffer
        _buf[start_in_output] = str_to_insert;
        _unassembled_bytes += str_to_insert.size();
    }

    //handle eof
    if(eof && _next == _output.bytes_written()){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    return _unassembled_bytes; 
}

bool StreamReassembler::empty() const {
    return _unassembled_bytes == 0;
}
