#ifndef COLD_NET_ENDIAN
#define COLD_NET_ENDIAN

#include <endian.h>

#include <cstdint>

namespace Cold::Endian {

inline uint16_t Host16ToNetwork16(uint16_t v) { return htobe16(v); }
inline uint16_t Network16ToHost16(uint16_t v) { return be16toh(v); }

inline uint32_t Host32ToNetwork32(uint32_t v) { return htobe32(v); }
inline uint32_t Network32ToHost32(uint32_t v) { return be32toh(v); }

inline uint64_t Host64ToNetwork64(uint64_t v) { return htobe64(v); }
inline uint64_t Network64ToHost64(uint64_t v) { return be64toh(v); }

}  // namespace Cold::Endian

#endif /* COLD_NET_ENDIAN */
