#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _time_since_last_segment_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
}

bool TCPConnection::active() const {
    return _active;
}

size_t TCPConnection::write(const string &data) {
    if(!active()){
        return 0;
    }
    size_t ret = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segments();
    return ret;

}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    //we need to send a FIN if we won't send data anymore
    _sender.fill_window();
    send_segments();
}

void TCPConnection::send_segments(){
    while(!_sender.segments_out().empty()){
        TCPSegment segment = move(_sender.segments_out().front());
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()){
            segment.header().ack = true;
            segment.header().ackno = _receiver.ackno().value();
        }
        //make sure window size will fit in 16 bits
        segment.header().win = min(_receiver.window_size(),
                              static_cast<size_t>(numeric_limits<uint16_t>::max()));
        _segments_out.push(move(segment));
    }
}

void TCPConnection::connect() {
    _sender.fill_window();
    send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
