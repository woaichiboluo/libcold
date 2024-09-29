#ifndef EXAMPLES_NET_TTCP_MESSAGE
#define EXAMPLES_NET_TTCP_MESSAGE

#include <cstdint>

struct SessionMessage {
  uint32_t number;
  uint32_t length;
} __attribute__((__packed__));

struct PayLoadMessage {
  uint32_t length;
  char data[0];
};

#endif /* EXAMPLES_NET_TTCP_MESSAGE */
