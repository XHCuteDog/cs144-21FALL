#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // If the receiver has announced a window size of zero, the fill window method should act like the window size is one.
    uint16_t windows_size = max(_remote_window_size, static_cast<uint16_t>(1));
    // Fill the window when possible
    while (_bytes_in_flight < windows_size) {
        //! Remember that the SYN and FIN flags also occupy a sequence number each, which means that they occupy space in the window.
        TCPSegment tcp_seg;
        if (!_syn_sent) {
            tcp_seg.header().syn = true;
            _syn_sent = true;
        }
        // it reads from its input ByteStream and sends as many bytes as possible in the form of TCPSegments, as long as
        // there are new bytes to be read and space available in the window. every TCPSegment you send fits fully inside
        // the receiver’s window. Make each individual TCPSegment as big as possible, but no bigger than the value given
        // by TCPConfig::MAX PAYLOAD SIZE (1452 bytes).
        size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE,
                                  min(_stream.buffer_size(), windows_size - _bytes_in_flight - tcp_seg.header().syn));
        auto payload = _stream.read(payload_size);
        tcp_seg.payload() = Buffer(std::move(payload));
        // If the stream has reached its end, and there are no more bytes to be sent, and the window is not full
        if (!_fin_sent && _stream.eof() && _bytes_in_flight + tcp_seg.length_in_sequence_space() < windows_size) {
            tcp_seg.header().fin = true;
            _fin_sent = true;
        }

        size_t len = tcp_seg.length_in_sequence_space();
        if (!len) {
            break;
        }

        tcp_seg.header().seqno = next_seqno();  // relative seqno
        _segments_out.emplace(tcp_seg);
        // Keep track of which segments have been sent but not yet acknowledged
        // we need to store the absolute sequence number of the segment in the queue, inorder to get the correct abs_seq
        // unwrap() just make sure return the latest correct abs_seq
        _outstanding_segments.emplace(std::make_pair(next_seqno_absolute(), tcp_seg));

        // Every time a segment containing data (nonzero length in sequence space) is sent (whether it’s the first time
        // or a retransmission), if the timer is not running, start it running so that it will expire after RTO
        // milliseconds
        if (!_timer.is_running()) {
            _timer.restart();
        }

        _next_seqno += len;
        _bytes_in_flight += len;
        if (_stream.buffer_empty()) {
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto ackno_abs = unwrap(ackno, _isn, next_seqno_absolute());
    if (ackno_abs > next_seqno_absolute()) {
        return;
    }
    bool is_ack = false;
    while (!_outstanding_segments.empty()) {
        auto [abs_seq, seg] = _outstanding_segments.front();
        // abs_seq + len - 1 means the last byte of the segment
        // If the receiver has announced a window size of zero, 
        if (abs_seq + seg.length_in_sequence_space() - 1 < ackno_abs) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _outstanding_segments.pop();
            is_ack = true;
        } else {
            break;
        }
    }
    // When the receiver gives the sender an ackno
    // (a) Set the RTO back to its “initial value.”
    // (b) If the sender has any outstanding data, restart the retransmission timer so that it will expire after RTO
    // milliseconds (for the current value of RTO). (c) Reset the count of “consecutive retransmissions” back to zero.
    if (is_ack) {
        _timer.set_rto(_initial_retransmission_timeout);
        if (!_outstanding_segments.empty()) {
            _timer.restart();
        }
        _consecutive_retransmissions = 0;
    }
    // When all outstanding data has been acknowledged, stop the retransmission timer.
    if (!bytes_in_flight()) {
        _timer.stop();
    }
    // the fill window method should act like the window size is one.
    _remote_window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick(ms_since_last_tick);
    // The TCPSender is responsible for looking through its collection of outstanding TCPSegments
    // and deciding if the oldest-sent segment has been outstanding for too long without acknowledgment.
    if (_timer.is_expired()) {
        _segments_out.emplace(_outstanding_segments.front().second);
        // do not pop from _outstanding_segments!!! maybe retransmit again
        // If the window size is nonzero:
        // i. Keep track of the number of consecutive retransmissions, and increment it
        // ii. Double the value of RTO.
        if (_remote_window_size > 0) {
            _consecutive_retransmissions++;
            _timer.set_rto(_timer.get_rto() * 2);
        }
        // Reset the retransmission timer and start it such that it expires after RTO milliseconds
        _timer.restart();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    // The TCPSender should generate and send a TCPSegment that has zero length in sequence space
    // Note: a segment like this one, which occupies no sequence numbers, doesn’t need to be kept track of as “outstanding” and won’t ever be retransmitted.
    TCPSegment tcp_seg;
    tcp_seg.header().seqno = next_seqno();
    tcp_seg.header().ack = true;
    _segments_out.emplace(tcp_seg);
}
