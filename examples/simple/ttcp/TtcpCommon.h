#ifndef SIMPLE_TTCP_TTCPCOMMON
#define SIMPLE_TTCP_TTCPCOMMON

#include <cstdint>

struct SessionMessage {
  uint32_t number;
  uint32_t length;
} __attribute__((__packed__));

struct PayloadMessage {
  uint32_t length;
  char data[0];
};

#endif /* SIMPLE_TTCP_TTCPCOMMON */
