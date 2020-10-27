#pragma once

#include "lmqtt_common.h"

namespace lmqtt {
    enum class property : uint8_t {
        PAYLOAD_FORMAT_INDICATOR                = 0x0,      // 
        MESSAGE_EXPIRY_INTERVAL                 = 0x0,      // 
        CONTENT_TYPE                            = 0x0,      // 
        RESPONSE_TOPIC                          = 0x0,      // 
        CORRELATION_DATA                        = 0x0,      // 
        SUBSCRIPTION_ID                         = 0x0,      // 
        SESSION_EXPIRY_INTERVAL                 = 0x0,      // 
        ASSIGNED_CLIENT_ID                      = 0x0,      // 
        SERVER_KEEP_ALIVE                       = 0x0,      // 
        AUTHENTICATION_METHOD                   = 0x0,      // 
        AUTHENTICATION_DATA                     = 0x0,      // 
        REQUEST_PROBLEM_INFORMATION             = 0x0,      // 
        WILL_DELAY_INTERVAL                     = 0x0,      // 
        REQUEST_RESPONSE_INFORMATION            = 0x0,      // 
        RESPONSE_INFORMATION                    = 0x0,      // 
        SERVER_REFERENCE                        = 0x0,      // 
        REASON_STRING                           = 0x0,      // 
        RECEIVE_MAXIMUM                         = 0x0,      // 
        TOPIC_ALIAS_MAXIMUM                     = 0x0,      // 
        TOPIC_ALIAS                             = 0x0,      // 
        MAXIMUM_QOS                             = 0x0,      // 
        RETAIN_AVAILABLE                        = 0x0,      // 
        USER_PROPERTY                           = 0x26,     // 
        MAXIMUM_PACKET_SIZE                     = 0x27,     // 
        WILDCARD_SUBSCRIPTION_AVAILABLE         = 0x28,     // 
        SUBSCRIPTION_ID_AVAILABLE               = 0x29,     // 
        SHARED_SUBSCRIPTION_AVAILABLE           = 0x2A,     // 
    };

} // namespace lmqtt
