#pragma once
#include "lmqtt_common.h"
#include "lmqtt_reason_codes.h"

namespace lmqtt {

    // bitwise or with upcoming byte to identify packet type
    enum class packet_type : uint8_t {
        //           VALUE                Direction            Description 
        RESERVED    = 0x0,        //   ***Forbidden***     ***Reserved***
        CONNECT     = 0x1,        //   Client --> Server   Connection request
        CONNACK     = 0x2,        //   Client <-- Server   Connect acknowledgment
        PUBLISH     = 0x3,        //   Client <-> Server   Publish message
        PUBACK      = 0x4,        //   Client <-> Server   Publish achnowledgment (QoS1)
        PUBREC      = 0x5,        //   Client <-> Server   Publish received (QoS 2 delivery part 1)
        PUBREL      = 0x6,        //   Client <-> Server   Publish release (QoS 2 delivery part 2)
        PUBCOMP     = 0x7,        //   Client <-> Server   Publish complete (QoS 2 delivery part 3)
        SUBSCRIBE   = 0x8,        //   Client --> Server   Subscribe request
        SUBACK      = 0x9,        //   Client <-> Server   Subscribe acknowledgment
        UNSUBSCRIBE = 0xA,        //   Client --> Server   Unsubscribe request
        UNSUBACK    = 0xB,        //   Client <-- Server   Unsubscribe acknowledgment
        PINGREQ     = 0xC,        //   Client --> Server   PING request
        PINGRESP    = 0xD,        //   Client <-- Server   PING response
        DISCONNECT  = 0xE,        //   Client <-> Server   Disconnect notification
        AUTH        = 0xF,        //   Client <-> Server   Authentication exchange
        UNKNOWN,
    };

    enum class packet_flag : uint8_t {
        //           VALUE                 FLAG          
        RESERVED    = 0x0,        //   ***Reserved***
        CONNECT     = 0x0,        //   ***Reserved***
        CONNACK     = 0xF,        //   | DUP | QoS | QoS | RETAIN
        PUBLISH     = 0x0,        //   ***Reserved***
        PUBACK      = 0x0,        //   ***Reserved***
        PUBREC      = 0x0,        //   ***Reserved***
        PUBREL      = 0x2,        //   ***Reserved***
        PUBCOMP     = 0x0,        //   ***Reserved***
        SUBSCRIBE   = 0x2,        //   ***Reserved***
        SUBACK      = 0x0,        //   ***Reserved***
        UNSUBSCRIBE = 0x2,        //   ***Reserved***
        UNSUBACK    = 0x0,        //   ***Reserved***
        PINGREQ     = 0x0,        //   ***Reserved***
        PINGRESP    = 0x0,        //   ***Reserved***
        DISCONNECT  = 0x0,        //   ***Reserved***
        AUTH        = 0x0,        //   ***Reserved***
    };

    struct fixed_header { 
        uint8_t _controlField;
        uint32_t _packetLen; // length without header
    };

    /*
     * Fixed Header: CONNACK
     * Fixed Header + Variable Header: PUBACK
     * Fixed Header + Variable Header + Payload: CONNECT
     */
    class packet {
        friend class connection;

        fixed_header _header {};
        std::vector<uint8_t> _varHeader {};
        std::vector<uint8_t> _payload {};
        packet_type _type = packet_type::UNKNOWN;
        
        [[nodiscard]] const reason_code create_fixed_header(const uint8_t rbyte) noexcept {
            uint8_t ptype = rbyte >> 4;
            uint8_t pflag = rbyte & 0xf;
            switch (static_cast<packet_type>(ptype)) {
            case packet_type::RESERVED:
                {
                // malformet packet
                return reason_code::MALFORMED_PACKET;
                }
                break;
            case packet_type::CONNECT:
                {
                _type = packet_type::CONNECT;
                // check for field
                if ((uint8_t)packet_flag::CONNECT != pflag) {
                    return reason_code::MALFORMED_PACKET;
                }
                }
                break;
            case packet_type::CONNACK:
                {
                    _type = packet_type::CONNACK;
                    // check for field
                    if ((uint8_t)packet_flag::CONNACK != pflag) {
                        return reason_code::MALFORMED_PACKET;
                    }
                }
                break;
            case packet_type::PUBLISH:
            case packet_type::PUBACK:
            case packet_type::PUBREC:
            case packet_type::PUBREL:
            case packet_type::PUBCOMP:
            case packet_type::SUBSCRIBE:
            case packet_type::SUBACK:
            case packet_type::UNSUBSCRIBE:
            case packet_type::UNSUBACK:
            case packet_type::PINGREQ:
            case packet_type::PINGRESP:
            case packet_type::DISCONNECT:
            case packet_type::AUTH:
                break;
            default:
                {
                    // malformet packet
                    return reason_code::MALFORMED_PACKET;
                }
                break;
            }
            _header._controlField = rbyte;
            return reason_code::SUCCESS;
        }

        [[nodiscard]] const reason_code decode_packet_length(const uint8_t rbyte, bool& next) noexcept {
            std::cout << "Reading " << std::bitset<8>(rbyte) << "(mul = " << std::hex << _mul << ")";
            _header._packetLen += rbyte & 0x7f * _mul;
            std::cout << "packet len = " << _header._packetLen << std::endl;
            if (_mul > 0x200000) { // 128 * 128 * 128
                return reason_code::MALFORMED_PACKET;
            }
            _mul *= 0x80; // next byte
            
            (rbyte & 0x80) ? next = true : next = false; // continuation bit, read next value
            
            return reason_code::SUCCESS;
        }

        friend std::ostream& operator << (std::ostream& os, const packet& packet) {
            os << "PAKCET_TYPE: " << packet.get_type_string() << ", FIXED_HEADER SIZE: " << sizeof(packet._header) << "\n";
            return os;
        }

    protected:
        const std::string_view get_type_string() const noexcept {
            switch (_type) {
            case packet_type::RESERVED:         return "RESERVED";
            case packet_type::CONNECT:          return "CONNECT";
            case packet_type::CONNACK:          return "CONNACK";
            case packet_type::PUBLISH:          return "PUBLISH";
            case packet_type::PUBACK:           return "PUBACK";
            case packet_type::PUBREC:           return "PUBREC";
            case packet_type::PUBREL:           return "PUBREL";
            case packet_type::PUBCOMP:          return "PUBCOMP";
            case packet_type::SUBSCRIBE:        return "SUBSCRIBE";
            case packet_type::SUBACK:           return "SUBACK";
            case packet_type::UNSUBSCRIBE:      return "UNSUBSCRIBE";
            case packet_type::UNSUBACK:         return "UNSUBACK";
            case packet_type::PINGREQ:          return "PINGREQ";
            case packet_type::PINGRESP:         return "PINGRESP";
            case packet_type::DISCONNECT:       return "DISCONNECT";
            case packet_type::AUTH:             return "AUTH";
            case packet_type::UNKNOWN:          return "UNKNOWN";
            default:                            return "NONE";
            }
            return ""; // keep the compiler happy
        }
    private:
        uint8_t _mul = 1; // multiplier for the variable byte integer decoder
    };

} // namespace lmqtt
