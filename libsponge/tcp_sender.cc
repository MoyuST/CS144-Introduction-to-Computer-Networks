#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if (_fin_sent && _unack_segmetns.empty() && _bytes_in_flight == 0) {
        return;
    }

    auto _receiver_window_used{_receiver_window};
    if (_receiver_window == 0) {
        _receiver_window_used = 1;
    }

    size_t maximum_sending_bytes =
        (_receiver_window_used >= _bytes_in_flight) ? _receiver_window_used - _bytes_in_flight : 0;

    bool send_fin_finally{false};
    size_t bytes_need_to_send = stream_in().buffer_size();
    size_t remain_send_bytesno{0};
    remain_send_bytesno = (maximum_sending_bytes > bytes_need_to_send) ? bytes_need_to_send : maximum_sending_bytes;

    if (!_syn_sent) {
        remain_send_bytesno++;
    }

    if (maximum_sending_bytes > bytes_need_to_send) {
        if (!_fin_sent && stream_in().input_ended()) {
            send_fin_finally = true;
            remain_send_bytesno++;
        }
    }

    bool send_non_empty_seg = false;

    while (remain_send_bytesno != 0) {
        TCPSegment data_seg{};
        size_t send_bytesno =
            (TCPConfig::MAX_PAYLOAD_SIZE <= remain_send_bytesno) ? TCPConfig::MAX_PAYLOAD_SIZE : remain_send_bytesno;
        remain_send_bytesno -= send_bytesno;
        data_seg.header().seqno = wrap(_next_seqno, _isn);
        if (!_syn_sent) {
            data_seg.header().syn = true;
            if (remain_send_bytesno != 0) {
                remain_send_bytesno--;
            } else {
                send_bytesno--;
            }
            _syn_sent = true;
        }
        if ((send_fin_finally) && (remain_send_bytesno <= 1)) {
            data_seg.header().fin = true;
            _fin_sent = true;
            remain_send_bytesno = 0;
        }
        data_seg.payload() = stream_in().read(send_bytesno);
        _next_seqno += data_seg.length_in_sequence_space();
        _segments_out.push(data_seg);
        _bytes_in_flight += data_seg.length_in_sequence_space();
        _unack_segmetns.insert({_next_seqno - 1, data_seg});
        send_non_empty_seg = true;
    }

    if (send_non_empty_seg && _inside_timer._running == false) {
        _inside_timer._running = true;
        _inside_timer._accumulate_ms = 0;
        _inside_timer._RTO = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if (!_syn_sent) {
        return;
    }

    uint64_t absolute_seqno = unwrap(ackno, _isn, _next_seqno);

    if (absolute_seqno > _next_seqno) {
        return;
    }

    bool pop_unack_storage = false;

    _receiver_window = window_size;

    for (auto it = _unack_segmetns.begin(); it != _unack_segmetns.end();) {
        if (absolute_seqno <= it->first) {
            break;
        }
        _bytes_in_flight -= it->second.length_in_sequence_space();
        auto _received_seg = it;
        it++;
        _unack_segmetns.erase(_received_seg);
        pop_unack_storage = true;
    }

    if (pop_unack_storage) {
        if (!_inside_timer._running) {
            _inside_timer._running = true;
        }
        _inside_timer._RTO = _initial_retransmission_timeout;
        _inside_timer._accumulate_ms = 0;
        _consecutive_retransmissions = 0;
    }

    if (_unack_segmetns.empty()) {
        _inside_timer._running = false;
        _consecutive_retransmissions = 0;
    }

    if (_unack_segmetns.empty() && stream_in().eof()) {
        _send_fin = true;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_inside_timer._running) {
        if (_inside_timer._accumulate_ms + ms_since_last_tick >= _inside_timer._RTO) {
            _segments_out.push(_unack_segmetns.begin()->second);
            _inside_timer._accumulate_ms = 0;
            if (_receiver_window != 0) {
                _inside_timer._RTO *= 2;
            }
            _consecutive_retransmissions++;
        } else {
            _inside_timer._accumulate_ms += ms_since_last_tick;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment data_seg{};
    data_seg.header().seqno = next_seqno();
    _segments_out.push(data_seg);
}
