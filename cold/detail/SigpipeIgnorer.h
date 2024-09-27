#ifndef COLD_DETAIL_SIGPIPEIGNORER
#define COLD_DETAIL_SIGPIPEIGNORER

#include <sys/signal.h>

namespace Cold::Detail {

class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, nullptr);
  }
};

inline void _IgnoreSigPipe() { static IgnoreSigPipe __; }

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_SIGPIPEIGNORER */
