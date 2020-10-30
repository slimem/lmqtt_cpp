#pragma once

#include "lmqtt_common.h"

namespace lmqtt {
enum class reason_code : uint8_t {
    SUCCESS                                 = 0x00,
    NO_SUBSCRIPTION_EXISTS                  = 0x11,
    UNSPECIFIED_ERROR                       = 0x80,
    MALFORMED_PACKET                        = 0x81,
    PROTOCOL_ERROR                          = 0x82,
    IMPLEMENTATION_SPECIFIC_ERROR           = 0x83,
    UNSUPPORTED_PROTOCOL_VERSION            = 0x84,
    NOT_AUTHORIZED                          = 0x87,
    PACKET_ID_IN_USE                        = 0x91,
    RECEIVE_MAXIMUM_EXCEEDED                = 0x93,
    PACKET_TOO_LARGE                        = 0x95,
    UNSUPPORTED_RETAIN                      = 0x9A,
    UNSUPPORTED_QOS                         = 0x9B,
    UNSUPPORTED_SHARED_SUBSCRIPTIONS        = 0x9E,
    UNSUPPORTED_SUBSCRIPTION_IDENTIFIERS    = 0xA1,
    UNSUPPORTED_WILDCARD_SUBSCRIPTIONS      = 0xA2,
};
} // namespace lmqtt