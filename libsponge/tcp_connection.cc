#include "tcp_connection.hh"

#include <iostream>
#include <limits>

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().avail_len(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::retrieve_seg_from_sender() {
    while (!_sender.segments_out().empty()) {
        auto seg_out = _sender.segments_out().front();
        _sender.segments_out().pop();
        seg_out.header().win = (_receiver.window_size() <= numeric_limits<uint16_t>::max())
                                   ? _receiver.window_size()
                                   : numeric_limits<uint16_t>::max();
        seg_out.header().ack = false;
        if (_receiver.ackno().has_value()) {
            seg_out.header().ackno = _receiver.ackno().value();
            seg_out.header().ack = true;
        }

        _segments_out.push(seg_out);
    }
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!active()) {
        return;
    }

    _time_since_last_segment_received = 0;

    if (seg.header().rst) {
        _sender.stream_in().set_error(true);
        _receiver.stream_out().set_error(true);
        _rst_received = true;
        _active = false;
        return;
    }

    if (_rst_received) {
        return;
    }

    _receiver.segment_received(seg);

    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    if (seg.header().syn && _sender.segments_out().empty()) {
        _sender.fill_window();
    }

    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) &&
        seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

    if (seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }

    retrieve_seg_from_sender();

    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) {
        if (!_linger_after_streams_finish) {
            _active = false;
        } else {
            _linger_after_streams_finish_start = true;
        }
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    if (!active()) {
        return 0;
    }

    size_t rt = _sender.stream_in().write(data);
    _sender.fill_window();
    retrieve_seg_from_sender();
    return rt;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    if (_linger_after_streams_finish_start && !_linger_after_streams_finish_finish) {
        if (_linger_after_streams_finish_timer + ms_since_last_tick >= 10 * _cfg.rt_timeout) {
            _linger_after_streams_finish_finish = true;
            _active = false;
            return;
        } else {
            _linger_after_streams_finish_timer += ms_since_last_tick;
        }
    }

    if (!active()) {
        return;
    }

    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        while (!_sender.segments_out().empty()) {
            _sender.segments_out().pop();
        }
        _sender.send_empty_segment();
        retrieve_seg_from_sender();
        _segments_out.front().header().rst = true;
        _rst_sent = true;
        _sender.stream_in().set_error(true);
        _receiver.stream_out().set_error(true);
        _active = false;
    }
    retrieve_seg_from_sender();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    retrieve_seg_from_sender();
}

void TCPConnection::connect() {
    _active = true;
    _sender.fill_window();
    retrieve_seg_from_sender();
}

size_t TCPConnection::ticked_time() { return _linger_after_streams_finish_timer; }

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            retrieve_seg_from_sender();
            _segments_out.front().header().rst = true;
            _rst_sent = true;
            _sender.stream_in().set_error(true);
            _receiver.stream_out().set_error(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
