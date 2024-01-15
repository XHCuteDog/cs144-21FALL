#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    auto header = seg.header();
    auto payload = seg.payload();
    if (header.syn) {
        _isn = header.seqno;
        _isn.set_init();
        _is_start = 1;
    }
    if (_isn.is_init()) {
        uint64_t abs_seqno = unwrap(header.seqno, _isn, _reassembler.stream_out().bytes_written());
        if (abs_seqno) {
            abs_seqno--;
        } else {
            if (re_send) {
                return;
            }
            re_send = true;
        }

        if (header.fin) {
            _is_eof = 1;
            _reassembler.push_substring(payload.copy(), abs_seqno, true);
        } else {
            _reassembler.push_substring(payload.copy(), abs_seqno, false);
        }
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.is_init()) {
        return std::nullopt;
    }
    size_t len = _reassembler.stream_out().bytes_written();
    len += _is_start;
    if (_reassembler.get_eof()) {
        len += _is_eof;
    }
    return wrap(len, _isn);
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
