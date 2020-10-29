#pragma once

#include "lmqtt_common.h"
#include "lmqtt_packet.h"

namespace lmqtt {

    //enum class packet_type : uint8_t;

    namespace property {
        enum class property_type : uint8_t {
            PAYLOAD_FORMAT_INDICATOR                = 0x01,     //  
            MESSAGE_EXPIRY_INTERVAL                 = 0x02,     // 
            CONTENT_TYPE                            = 0x03,     // 
            RESPONSE_TOPIC                          = 0x08,     // 
            CORRELATION_DATA                        = 0x09,     // 
            SUBSCRIPTION_ID                         = 0x0B,     // 
            SESSION_EXPIRY_INTERVAL                 = 0x11,     // 
            ASSIGNED_CLIENT_ID                      = 0x12,     // 
            SERVER_KEEP_ALIVE                       = 0x13,     // 
            AUTHENTICATION_METHOD                   = 0x15,     // 
            AUTHENTICATION_DATA                     = 0x16,     // 
            REQUEST_PROBLEM_INFORMATION             = 0x17,     // 
            WILL_DELAY_INTERVAL                     = 0x18,     // 
            REQUEST_RESPONSE_INFORMATION            = 0x19,     // 
            RESPONSE_INFORMATION                    = 0x1A,     // 
            SERVER_REFERENCE                        = 0x1C,     // 
            REASON_STRING                           = 0x1F,     // 
            RECEIVE_MAXIMUM                         = 0x21,     // 
            TOPIC_ALIAS_MAXIMUM                     = 0x22,     // 
            TOPIC_ALIAS                             = 0x23,     // 
            MAXIMUM_QOS                             = 0x24,     // 
            RETAIN_AVAILABLE                        = 0x25,     // 
            USER_PROPERTY                           = 0x26,     // 
            MAXIMUM_PACKET_SIZE                     = 0x27,     // 
            WILDCARD_SUBSCRIPTION_AVAILABLE         = 0x28,     // 
            SUBSCRIPTION_ID_AVAILABLE               = 0x29,     // 
            SHARED_SUBSCRIPTION_AVAILABLE           = 0x2A,     //
            UNKNOWN
        };

        enum class data_type : uint8_t {
            BYTE,
            TWO_BYTES_INT,
            FOUR_BYTES_INT,
            VARIABLE_BYTE_INT,
            UTF8_STRING,
            UTF8_STRING_PAIR,
            BINARY,
            UNKNOWN
        };

        class property_data_proxy {
            ~property_data_proxy() = default;
        };

        template <typename T>
        class property_data : public property_data_proxy {
            T _data;
            T& get_data() const noexcept {
                return T;
            }
            void set_data(T& data) noexcept {
                _data = data;
            }
        };

        struct utils {
            static constexpr const data_type get_property_data_type(const property_type& property) noexcept {
                switch (property) {
                case property_type::PAYLOAD_FORMAT_INDICATOR:
                case property_type::REQUEST_PROBLEM_INFORMATION:
                case property_type::REQUEST_RESPONSE_INFORMATION:
                case property_type::MAXIMUM_QOS:
                case property_type::RETAIN_AVAILABLE:
                case property_type::WILDCARD_SUBSCRIPTION_AVAILABLE:
                case property_type::SUBSCRIPTION_ID_AVAILABLE:
                case property_type::SHARED_SUBSCRIPTION_AVAILABLE:
                    return data_type::BYTE;
                case property_type::SERVER_KEEP_ALIVE:
                case property_type::RECEIVE_MAXIMUM:
                case property_type::TOPIC_ALIAS_MAXIMUM:
                case property_type::TOPIC_ALIAS:
                    return data_type::TWO_BYTES_INT;
                case property_type::MESSAGE_EXPIRY_INTERVAL:
                case property_type::SESSION_EXPIRY_INTERVAL:
                case property_type::WILL_DELAY_INTERVAL:
                case property_type::MAXIMUM_PACKET_SIZE:
                    return data_type::FOUR_BYTES_INT;
                case property_type::SUBSCRIPTION_ID:
                    return data_type::VARIABLE_BYTE_INT;
                case property_type::CONTENT_TYPE:
                case property_type::RESPONSE_TOPIC:
                case property_type::ASSIGNED_CLIENT_ID:
                case property_type::AUTHENTICATION_METHOD:
                case property_type::RESPONSE_INFORMATION:
                case property_type::SERVER_REFERENCE:
                case property_type::REASON_STRING:
                    return data_type::UTF8_STRING;
                case property_type::USER_PROPERTY:
                    return data_type::UTF8_STRING_PAIR;
                case property_type::CORRELATION_DATA:
                case property_type::AUTHENTICATION_DATA:
                    return data_type::BINARY;
                default:
                    return data_type::UNKNOWN;
                }
                // keep the compiler happy
                return data_type::UNKNOWN;
            }

            // Check if the packet type really supports this property type
            static constexpr const bool validate_packet_property_type(
                const property_type& propertyType,
                const packet_type& packetType
            ) noexcept {
                switch (propertyType) {
                case property_type::PAYLOAD_FORMAT_INDICATOR:
                    return packetType == packet_type::PUBLISH;
                case property_type::MESSAGE_EXPIRY_INTERVAL:
                    return packetType == packet_type::PUBLISH;
                case property_type::CONTENT_TYPE:
                    return packetType == packet_type::PUBLISH;
                case property_type::RESPONSE_TOPIC:
                    return packetType == packet_type::PUBLISH;
                case property_type::CORRELATION_DATA:
                    return packetType == packet_type::PUBLISH;
                case property_type::SUBSCRIPTION_ID:
                    return (packetType == packet_type::PUBLISH)
                        || (packetType == packet_type::SUBSCRIBE);
                case property_type::SESSION_EXPIRY_INTERVAL:
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK)
                        || (packetType == packet_type::DISCONNECT);
                case property_type::ASSIGNED_CLIENT_ID:
                    return packetType == packet_type::CONNACK;
                case property_type::SERVER_KEEP_ALIVE:
                    return packetType == packet_type::CONNACK;
                case property_type::AUTHENTICATION_METHOD:
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK)
                        || (packetType == packet_type::AUTH);
                case property_type::AUTHENTICATION_DATA:
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK)
                        || (packetType == packet_type::AUTH);
                case property_type::REQUEST_PROBLEM_INFORMATION:
                    return packetType == packet_type::CONNECT;
                case property_type::WILL_DELAY_INTERVAL:
                    return false; // until will properties are supported
                case property_type::REQUEST_RESPONSE_INFORMATION:
                    return packetType == packet_type::CONNECT;
                case property_type::RESPONSE_INFORMATION:
                    return packetType == packet_type::CONNACK;
                case property_type::SERVER_REFERENCE:
                    return (packetType == packet_type::CONNACK)
                        || (packetType == packet_type::DISCONNECT);
                case property_type::REASON_STRING:
                    return (packetType == packet_type::CONNACK)
                        || (packetType == packet_type::PUBACK)
                        || (packetType == packet_type::PUBREC)
                        || (packetType == packet_type::PUBREL)
                        || (packetType == packet_type::PUBCOMP)
                        || (packetType == packet_type::SUBACK)
                        || (packetType == packet_type::UNSUBACK)
                        || (packetType == packet_type::DISCONNECT)
                        || (packetType == packet_type::AUTH);
                case property_type::RECEIVE_MAXIMUM:
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK);
                case property_type::TOPIC_ALIAS_MAXIMUM:
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK);
                case property_type::TOPIC_ALIAS:
                    return packetType == packet_type::PUBLISH;
                case property_type::MAXIMUM_QOS:
                    return packetType == packet_type::CONNACK;
                case property_type::RETAIN_AVAILABLE:
                    return packetType == packet_type::CONNACK;
                case property_type::USER_PROPERTY:
                    // TODO: SUPPORT WILL PROPERTIES SOMEHOW
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK)
                        || (packetType == packet_type::PUBLISH)
                        || (packetType == packet_type::PUBACK)
                        || (packetType == packet_type::PUBREC)
                        || (packetType == packet_type::PUBREL)
                        || (packetType == packet_type::PUBCOMP)
                        || (packetType == packet_type::SUBSCRIBE)
                        || (packetType == packet_type::SUBACK)
                        || (packetType == packet_type::UNSUBSCRIBE)
                        || (packetType == packet_type::UNSUBACK)
                        || (packetType == packet_type::DISCONNECT)
                        || (packetType == packet_type::AUTH);
                case property_type::MAXIMUM_PACKET_SIZE:
                    return (packetType == packet_type::CONNECT)
                        || (packetType == packet_type::CONNACK);
                case property_type::WILDCARD_SUBSCRIPTION_AVAILABLE:
                    return packetType == packet_type::CONNACK;
                case property_type::SUBSCRIPTION_ID_AVAILABLE:
                    return packetType == packet_type::CONNACK;
                case property_type::SHARED_SUBSCRIPTION_AVAILABLE:
                    return packetType == packet_type::CONNACK;
                default:
                    return false;
                }
                // keep the compiler happy
                return false;
            }
        };
    } //namespace property

} // namespace lmqtt
