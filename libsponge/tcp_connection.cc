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

    if (!_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0) {
        // at this time, TCP acts as a server,
        // and should not receive any segment except it has SYN flag
        if (!header.syn) {
            return;
        }
        _receiver.segment_received(seg);
        // try to send SYN segment, use for opening TCP at the same time
        connect();
        return;
    }

    if(header.rst){ //if we received a RST
        unclean_shutdown();
        return;
    }

    //pass seg to receiver to handle it and reassemble it to the inbound stream
    _receiver.segment_received(seg);

    //recall that ALL tcp segments should have an ackno, except the
    //very first SYN sent in the three-way handshake,
    //and if the segment is indeed a SYN,
    //_receiver.segment_received() that we just called had already handle it
    if(!_receiver.ackno().has_value()){
        return;
    }

    //if we didn't finish sending, but received a FIN(remote peer finish sending)
    //this means that we will be the passive closer
    if(!_sender.stream_in().eof() && _receiver.stream_out().input_ended()){
        _linger_after_streams_finish = false;   //passive closer dont linger
    }

    //pass ackno and window size to sender to update our sending window
    if(header.ack){
        _sender.ack_received(header.ackno, header.win);
    }

    _sender.fill_window();
    //if we received an segment, but don't have real data to send
    if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()){
        _sender.send_empty_segment();   //then send an empty ACK
    }
    
    //if received segment has no data and seqno
    //and seqno is the ackno the remote host expected - 1
    //then it is a keep-alive segment
    if(seg.length_in_sequence_space() == 0 && header.seqno == _receiver.ackno().value() - 1){
        _sender.send_empty_segment();   //send ACK, responding keep alive
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
    if(!_active){
        return;
    }
    
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
        && (!_linger_after_streams_finish   //4A.we are the Passive Closer
        || _time_since_last_segment_received >= 10 * _cfg.rt_timeout)){
            //or lingering time is over
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
        _segments_out.emplace(move(segment));
    }
}

void TCPConnection::send_RST() {
    //generate a empty segment
    _sender.send_empty_segment();
    //set RST flag and send
    TCPSegment segment = move(_sender.segments_out().front());
    _sender.segments_out().pop();
    segment.header().rst = true;
    _segments_out.emplace(move(segment));

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
            send_RST();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
