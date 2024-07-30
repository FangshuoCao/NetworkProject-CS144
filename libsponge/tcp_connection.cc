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
    if(!active()){
        return;
    }
    //reset timer
    _time_since_last_segment_received = 0;
    const TCPHeader &header = seg.header();

    if(header.rst){ //if we received a RST
        unclean_shutdown();
        return;
    }

    //pass seg to receiver to handle it
    _receiver.segment_received(seg);

    if(!_receiver.ackno().has_value()){
        return;
    }

    if(!_sender.stream_in().eof() && _receiver.stream_out().input_ended()){
        _linger_after_streams_finish = false;
    }

    //update our sending window
    if(header.ack){
        _sender.ack_received(header.ackno, header.win);
    }
    _sender.fill_window();

    if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()){
        _sender.send_empty_segment();
    }
    
    if(seg.length_in_sequence_space() == 0 && header.seqno == _receiver.ackno().value() - 1){
        _sender.send_empty_segment();
    }
    send_segments();
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
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    //if we've retransmitted too many times, sth very bad has happened
    //so we need to reset the connection
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        send_RST();
        return;
    }

    //perform clean shutdown if all 4 prerequisite is met
    if(_receiver.stream_out().input_ended() //1.inbound stream fully assembled and ended
        && _sender.stream_in().eof()        //2.outbut stream ended
        && _sender.bytes_in_flight() == 0   //3.outbound stream fully acked by peer
        && (!_linger_after_streams_finish   //4A.lingering after both stream end
        || _time_since_last_segment_received > 10 * _cfg.rt_timeout)){ //4B.Passive close
            _active = false;    //close the connction
            return;
    }

    //recall that TCPSender's tick() will fill window if it need to retransmit sth
    //therefore we need to send out segments if there's any
    send_segments();
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

void TCPConnection::send_RST() {
    _sender.fill_window();
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    TCPSegment segment = _sender.segments_out().front();
    _sender.segments_out().pop();
    optional<WrappingInt32> ackno = _receiver.ackno();
    if (ackno.has_value()) {
        segment.header().ack = true;
        segment.header().ackno = ackno.value();
    }
    segment.header().win = _receiver.window_size() <= numeric_limits<uint16_t>::max()
                               ? _receiver.window_size()
                               : numeric_limits<uint16_t>::max();
    segment.header().rst = true;
    _segments_out.emplace(segment);

    unclean_shutdown();
}


void TCPConnection::connect() {
    _sender.fill_window();
    send_segments();
}

void TCPConnection::unclean_shutdown(){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
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
