#include "tcp_receiver.hh"
#include <iostream>

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

#define TWOTO32 4294967296

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(!_syn_received && seg.header().syn){
        _syn = seg.header().seqno;
        _syn_received = true;
    }

    if(!_syn_received){
        return;
    }

    uint64_t absolute_idx{unwrap(seg.header().seqno, _syn, _reassembler.first_reassembled_idx())};

    if(!_fin_received && seg.header().fin){
        _fin = absolute_idx+seg.length_in_sequence_space()-1;
        _fin_received = true;
        _reassembler.push_substring(
            "", 
            _fin-1,
            true
        );    
    }

    if(!seg.header().syn && absolute_idx==0){
        return;
    }

    _reassembler.push_substring(
        seg.payload().copy(), 
        seg.header().syn ? 0 : absolute_idx-1,
        false
    );
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(_syn_received){
        if(stream_out().input_ended()){
            return wrap(_fin, _syn)+1; 
        }
        else{
            return wrap(_reassembler.first_reassembled_idx(), _syn)+1;
        }
    }
    return nullopt; 
}

size_t TCPReceiver::window_size() const {
    return _capacity-stream_out().buffer_size();
}