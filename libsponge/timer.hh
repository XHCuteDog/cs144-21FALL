#include <cstddef>
// You’ll implement the retransmission timer: an alarm that can be started at a certain time, and the alarm goes off (or “expires”) once the RTO has elapsed.
// implementing the functionality of the retransmission timer in a separate class,
class Timer {
  private:
    unsigned int _RTO{0};
    unsigned int _time_count{0};
    bool _is_running{false};

  public:
    Timer() = default;
    Timer(unsigned int rto) : _RTO(rto){};
    void restart() {
        _time_count = 0;
        _is_running = true;
    }
    void stop() { _is_running = false; }
    bool is_running() const { return _is_running; }
    void tick(const size_t ms_since_last_tick) {
        if (_is_running) {
            _time_count += ms_since_last_tick;
        }
    }
    bool is_expired() const { return _is_running && _time_count >= _RTO; }
    void set_rto(const unsigned int rto) { _RTO = rto; }
    unsigned int get_rto() const { return _RTO; }
    ~Timer() = default;
};
