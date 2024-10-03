#ifndef SPONGE_LIBSPONGE_TIMER_HH
#define SPONGE_LIBSPONGE_TIMER_HH

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <queue>

using namespace std;

//! \brief A timer that keeps track of the retransmission timeout.
class Timer {
  private:
    WrappingInt32 _isn;                                 //! our initial sequence number, the number for our SYN.
    bool _running{false};                               //! Flag indicating that the timer is running.
    unsigned int _initial_retransmission_timeout;       //! The initial retransmission timeout.
    unsigned int _rto;                                  //! The retransmission timeout.
    unsigned int _num_retransmissions{0};               //! The number of consecutive retransmissions.
    unsigned int _time_elapsed{0};                      //! The time elapsed since the last retransmission.
    queue<pair<uint64_t, TCPSegment>> _outstandings{};  //! The outstanding segments.
    uint64_t _nx_seqno{0};                              //! The (absolute) sequence number for the byte to be sent next.
    uint64_t _num_acked{0};                             //! The number of bytes acknowledged.

  public:
    //! Initialize a Timer.
    Timer(const unsigned int rto, WrappingInt32 isn) : _isn(isn), _initial_retransmission_timeout(rto), _rto(rto) {};

    //! \name Methods that can cause the Timer to retransmit a segment
    //!@{

    //! \brief Track a segment.
    void track(TCPSegment &seg);

    //! \brief Remove segments that have been acknowledged.
    bool remove_received(const WrappingInt32 ackno);

    //! \brief Indicate whether a segment should be resent.
    bool resend(const size_t ms_since_last_tick);

    //! \brief Extend the retransmission timeout.
    void extend_rto() {
        _num_retransmissions++;
        _rto *= 2;
    };
    //!@}

    //! \name Accessors
    //!@{
    bool running() const { return _running; }

    unsigned int rto() const { return _rto; }

    unsigned int num_retransmissions() const { return _num_retransmissions; }

    unsigned int time_elapsed() const { return _time_elapsed; }

    uint64_t next_seqno() const { return _nx_seqno; }

    queue<pair<uint64_t, TCPSegment>> outstandings() const { return _outstandings; }

    uint64_t num_acked() const { return _num_acked; }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TIMER_HH