#pragma once

namespace lmqtt {

// TODO: Replace by std::error_code in the future which
// holds the error ID and error description, which helps
// with REASON_STRING property
enum class reason_code : uint8_t {
    SUCCESS                                 = 0x00,
    DISCONNECT_WITH_WILL_MESSAGE            = 0x04,
    NO_SUBSCRIPTION_EXISTS                  = 0x11,
    UNSPECIFIED_ERROR                       = 0x80,
    MALFORMED_PACKET                        = 0x81,
    PROTOCOL_ERROR                          = 0x82,
    IMPLEMENTATION_SPECIFIC_ERROR           = 0x83,
    UNSUPPORTED_PROTOCOL_VERSION            = 0x84,
    NOT_AUTHORIZED                          = 0x87,
    SERVER_BUSY                             = 0x89,
    SERVER_SHUTTING_DOWN                    = 0x8B,
    KEEP_ALIVE_TIMEOUT                      = 0x8D,
    SESSION_TAKEN_OVER                      = 0x8E,
    TOPIC_FILTER_INVALID                    = 0x90,
    PACKET_ID_IN_USE                        = 0x91,
    RECEIVE_MAXIMUM_EXCEEDED                = 0x93,
    TOPIC_ALIAS_INVALID                     = 0x94,
    PACKET_TOO_LARGE                        = 0x95,
    MESSAGE_RATE_TOO_HIGH                   = 0x96,
    QUOTA_EXCEEDED                          = 0x97,
    ADMINISTRATIVE_ACTION                   = 0x98,
    PAYLOAD_FORMAT_INVALID                  = 0x99,
    UNSUPPORTED_RETAIN                      = 0x9A,
    UNSUPPORTED_QOS                         = 0x9B,
    USE_ANOTHER_SERVER                      = 0x9C,
    SERVER_MOVED                            = 0x9D,
    UNSUPPORTED_SHARED_SUBSCRIPTIONS        = 0x9E,
    CONNECTION_RATE_EXCEEDED                = 0x9F,
    UNSUPPORTED_SUBSCRIPTION_IDENTIFIERS    = 0xA1,
    UNSUPPORTED_WILDCARD_SUBSCRIPTIONS      = 0xA2,
};

enum class [[nodiscard]] return_code : uint8_t {
    OK      = 0,
    FAIL    = 1
};

} // namespace lmqtt