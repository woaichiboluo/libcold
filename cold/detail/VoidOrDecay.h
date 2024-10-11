#ifndef COLD_DETAIL_VOIDORDECAY
#define COLD_DETAIL_VOIDORDECAY

#include <type_traits>

namespace Cold::Detail {

struct VoidType {};

template <typename T>
using VoidOrDecay =
    std::conditional_t<std::is_void_v<T>, VoidType, std::decay_t<T>>;

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_VOIDORDECAY */
