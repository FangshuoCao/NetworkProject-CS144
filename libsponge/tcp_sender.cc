#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _RTO(retx_timeout){}


uint64_t TCPSender::bytes_in_flight() const { return _outstanding_bytes; }


void TCPSender::fill_window() {


    //zero window probing - when filling the window, if window size is 0, act like it is 1
    //the last byte we need to send is ackno + window_size,
    //and first byte to send if next_seqno, so bytes_to_send is their difference
    size_t bytes_to_send = _ackno_abs + (_window_size == 0 ? 1 : _window_size) - _next_seqno;

    while(bytes_to_send > 0 && !_stop_sending){
        TCPSegment segment;
        TCPHeader &header = segment.header();

        //_next_seqno == 0 means connection is not established yet, and this
	        //is the first message in the threeway handshake, so just send a SYN
	    if(_next_seqno == 0){
	        header.syn = true;
	        --bytes_to_send;    //SYN take up one space in the window
        }

        header.seqno = wrap(_next_seqno, _isn);
        Buffer &buf = segment.payload();
        buf = stream_in().read(min(bytes_to_send, TCPConfig::MAX_PAYLOAD_SIZE));
        bytes_to_send -= buf.size();

        //if we reach eof, set FIN in the segment
        //but we shouldn't send FIN if it makes the segment exceeds the windod size
        //instead, send FIN in a separate segment
        if(stream_in().eof() && bytes_to_send > 0){
            header.fin = true;
            --bytes_to_send;    //FIN take up one space in the window
            _stop_sending = true;
        }

        size_t len = segment.length_in_sequence_space();
        if(len == 0){
            return;
        }

        //send segment
        segments_out().emplace(segment);
        if(!_timer.started()){
            _timer.start();
        }

        //push segment into oustanding segments and start timer
        _outstanding_segments.emplace(segment);
        

        //update corresponding fields
        _next_seqno += len;
        _outstanding_bytes += len;
    }
}



//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ackno_abs = unwrap(ackno, _isn, _next_seqno);
    if(ackno_abs > _next_seqno){
        return;     //invalid ackno: ackno is a byte that we haven't send yet
    }
    _ackno_abs = ackno_abs;
    _window_size = window_size;

    bool new_data_acked = false;
    
    //popping segments from the outstanding queue if it is received
    while(!_outstanding_segments.empty()){
        TCPSegment segment = _outstanding_segments.front();
        size_t len = segment.length_in_sequence_space();
        uint64_t seqno = unwrap(segment.header().seqno, _isn, _next_seqno);

        //if a segment is not fully ACKed(all bytes in it are ACKed), don't pop it
        if(seqno + len > _ackno_abs){
            break;
        }
        new_data_acked = true;
        _outstanding_segments.pop();
        _outstanding_bytes -= len;
    }

    if(new_data_acked){
        //reset RTO
        _RTO = _initial_retransmission_timeout;
        if(!_outstanding_segments.empty()){
            _timer.start(); //if there are still outstanding segments, reset timer
        }else{
            _timer.stop();  //if there is no outstanding segments, stop timer
        }
        _consec_retrans = 0;    //reset count of consecutive retransmission
    }

    fill_window();

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    //if timer didn't expire, do nothing 
    if(_timer.expired(ms_since_last_tick, _RTO)){
        //don't pop! since we might need to retransmit it again
        segments_out().push(_outstanding_segments.front());
        if(_window_size != 0){
            _consec_retrans++;
            _RTO *= 2;
        }
        _timer.start();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consec_retrans; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    segments_out().emplace(segment);
}
