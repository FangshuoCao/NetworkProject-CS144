#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {

    const TCPHeader &header = seg.header();

    if(!_syn_received){ //current state is LISTENING
        if(!header.syn){
            return; //don't accept any data until we received a SYN
        }
        //received a SYN
        _syn_received = true;   //set state to ESTABLISHED
        _isn = header.seqno;   //set ISN
    }

    //checkpoint used to unwrap is the index of last byte written to output stream
    uint64_t checkpoint = _reassembler.stream_out().bytes_written() + 1;

    //convert seqno to seqno_abs
    uint64_t seqno_abs = unwrap(header.seqno, _isn, checkpoint);

    //convert seqno_abs to stream index
    //we need to distinguish between SYN and regular segment
    //see lab report for explanation
    uint64_t stream_index = header.syn ? seqno_abs : seqno_abs - 1;

    //push payload into reassembler
    //FIN is set means EOF is set, therefore we already implement 
    //the logic to handle FIN in our reassembler
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    //if state is LISTENING, ackno is null since we didn't receive a SYN yet
    if(!_syn_received){
        return nullopt;
    }

    // +1 for SYN
    uint64_t ackno_abs = _reassembler.stream_out().bytes_written() + 1;

    //if we have received a FIN, +1 for FIN
    if(_reassembler.stream_out().input_ended()){
        ackno_abs++;
    }

    return wrap(ackno_abs, _isn);
}

size_t TCPReceiver::window_size() const {
    //full capacity - number of bytes reassembled but not read by application
    return _capacity - _reassembler.stream_out().buffer_size();
}
