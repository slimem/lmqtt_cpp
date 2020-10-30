#pragma once

#include "lmqtt_common.h"
#include "lmqtt_reason_codes.h"
#include "lmqtt_types.h"

namespace lmqtt {

struct fixed_header { 
    uint8_t _controlField;
    uint32_t _packetLen; // length without header
    constexpr size_t size() const noexcept {
        if (_packetLen >= 0x200000) return 5; // control field + 4 bytes
        else if (_packetLen >= 0x4000) return 4; // control field + 3 bytes
        else if (_packetLen >= 0x80) return 3; // control flield + 2 bytes
        else return 2;
    }
};

/*
 * Fixed Header: CONNACK
 * Fixed Header + Variable Header: PUBACK
 * Fixed Header + Variable Header + Payload: CONNECT
 */
class packet {
    friend class connection;

    fixed_header _header {};
    //std::vector<uint8_t> _varHeader {};
    //std::vector<uint8_t> _pyload {};
    std::vector<uint8_t> _body;
    packet_type _type = packet_type::UNKNOWN;
    
    [[nodiscard]] const reason_code create_fixed_header() noexcept {
        uint8_t ptype = _header._controlField >> 4;
        uint8_t pflag = _header._controlField & 0xf;
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
        return reason_code::SUCCESS;
    }

    [[nodiscard]] const reason_code decode_packet_body() {
        // for now, only decode CONNECT packet
        std::cout << "decoding packet body\n";
        switch (_type) {
        case packet_type::CONNECT:
        {
            return decode_connect_packet();
        }
        }
        return reason_code::SUCCESS;
    }

    [[nodiscard]] const reason_code decode_connect_packet() {
        // byte 0 : length MSB
        // byte 1 : length LSB
        const uint8_t protocolNameLen = _body[1] - _body[0];
        if (protocolNameLen != 0x4) {
            return reason_code::MALFORMED_PACKET;
        }

        {
            // bytes 2 3 4 5 : MQTT protocol
            const uint8_t protocolOffset = 2;
            char mqttStr[4];
            std::memcpy(mqttStr, _body.data() + protocolOffset, 4);
            // compare non null terminated string
            if (std::strncmp(mqttStr, "MQTT", 4)) {
                // ~~ [MQTT-3.1.2-2]
                return reason_code::UNSUPPORTED_PROTOCOL_VERSION;
            }
        }

        // byte 6 : MQTT version
        if (_body[6] != 0x5) {
            return reason_code::UNSUPPORTED_PROTOCOL_VERSION;
        }

        // byte 7 : Connect flags
        {
            const uint8_t flags = _body[7];
            // bit 0 : Reserved : first check the reserved flag is set to 0
            // ~~ [MQTT-3.1.2-3]
            if (flags & 0x1) {
                return reason_code::MALFORMED_PACKET;
            }

            // Extract the reset of the flags with bitmasking
            //TODO: Replace with bitfield in the future
            // bit 1 : Clean start
            const uint8_t cleanStart = (flags & 0x2) >> 1;
            // bit 2 : Will Flag
            const uint8_t willFlag = (flags & 0x4) >> 2;
            // bit 3 and 4 : Will QoS
            uint8_t willQoS = (flags & 0x18) >> 3;
            // bit 5 : Will retain
            const uint8_t willRetain = (flags & 0x20) >> 5;
            // bit 6 : Password Flag
            const uint8_t passwordFlag = (flags & 0x40) >> 6;
            // bit 7 : User name flag
            const uint8_t userNameFlag = (flags & 0x80) >> 7;

            // check if willQos is valid
            if (willQoS == 0x3) {
                return reason_code::MALFORMED_PACKET;
            }
            if (!willFlag) {
                willQoS = 0;
            }
        }

        // byte 8 and 9 : Keep alive MSB and LSB
        const uint16_t keepAlive = (_body[8] << 8) | _body[9];

        // check if body size can hold a maximum variable length int (base + 3)
        if (_body.size() < 13) { // starts at 10 and ends at 13
            return reason_code::MALFORMED_PACKET;
        }
        // now compute the variable
        uint32_t propertyLength = 0;
        uint8_t varIntoffset = 0; // length of the variable in the buffer
        // here, we are pretty comfortable that the buffer size is more than 13
        decode_variable_int(_body.data() + 10, propertyLength, varIntoffset);
        std::cout << "Decoded variable size " << propertyLength << std::endl;

        decode_properties(10 + varIntoffset, propertyLength);
        uint8_t propertyStart = 10 + varIntoffset;

        //std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        //std::chrono::system_clock::time_point timeThen;
        //msg >> timeThen;
        //std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";

        return reason_code::SUCCESS;
    }

    // TODO: move to a util class
    // This is an util method (unlike decode_packet_length). It takes a buffer, a reference to a decoded value
    // and a reference to an offset. The offset will be used to know from where to start the next reading after
    // decoding the variable.
    static const reason_code decode_variable_int(uint8_t* buffer, uint32_t& decodedValue, uint8_t& offset) {
        decodedValue = 0;
        uint8_t mul = 1;

        for (uint8_t offset = 0; offset < 4; ++offset) {
            decodedValue += buffer[offset] & 0x7f * mul;
            if (mul > 0x200000) { // 128 * 128 * 128
                //return reason_code::MALFORMED_PACKET;
            }
            mul *= 0x80; // prepare for next byte
            // no continuation bit, break from the loop
            if (!(buffer[offset] & 0x80)) break;
        }

        offset++;
        return reason_code::SUCCESS;
    }

    // this method is a special case since it it used to decode the packet length, where the asio buffer
    // reads one byte at a time. So if the "next" argument is true, it means that we should read another
    // byte and so on. If we read more than 4 (_mul is multiplied 4 times) it means that the packet is
    // malformed.
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

    const reason_code decode_properties(uint8_t start, uint32_t length) {
        // first, check if the _body can hold this data
        if (_body.size() < (start + length)) {
            return reason_code::MALFORMED_PACKET;
        }
        
        uint8_t* buff = _body.data() + start;
        
        // start decoding
        uint32_t index = 0;
        while (index != length) {
            const property::property_type ptype = static_cast<property::property_type>(buff[index]);
            // check if this packet type supports this property
            if (!property::utils::validate_packet_property_type(ptype, _type)) {
                // check this reason code
                return reason_code::MALFORMED_PACKET;
            }




            break;
        }


        return reason_code::SUCCESS;
    }

    friend std::ostream& operator << (std::ostream& os, const packet& packet) {
        os << "PAKCET_TYPE: " << packet.get_type_string() << ", FIXED_HEADER SIZE: " << sizeof(packet._header) << "\n";
        return os;
    }

public:
    size_t size() const noexcept {
        return _header.size() + _body.size();
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

    [[nodiscard]] bool has_packet_id() const noexcept {
        switch (_type) {
        case packet_type::CONNECT:          return false;
        case packet_type::CONNACK:          return false;
        case packet_type::PUBLISH:          return _qos > 0 ? true : false; // true of QoS > 0
        case packet_type::PUBACK:           return true;
        case packet_type::PUBREC:           return true;
        case packet_type::PUBREL:           return true;
        case packet_type::PUBCOMP:          return true;
        case packet_type::SUBSCRIBE:        return true;
        case packet_type::SUBACK:           return true;
        case packet_type::UNSUBSCRIBE:      return true;
        case packet_type::UNSUBACK:         return true;
        case packet_type::PINGREQ:          return false;
        case packet_type::PINGRESP:         return false;
        case packet_type::DISCONNECT:       return false;
        case packet_type::AUTH:             return false;
        default:                            return false;
        }
        return false; // keep the compiler happy
    }
private:
    uint8_t _mul = 1; // multiplier for the variable byte integer decoder
    uint8_t _qos = 0;
    uint8_t _varIntBuff[4]; // a buffer to decode variable int
};



} // namespace lmqtt
