#pragma once

#include "lmqtt_common.h"
#include "lmqtt_reason_codes.h"
#include "lmqtt_types.h"
#include "lmqtt_properties.h"
#include "lmqtt_payload.h"
#include "lmqtt_utils.h"

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
            return decode_connect_packet_body();
        }
        return reason_code::SUCCESS;
    }

    [[nodiscard]] const reason_code decode_connect_packet_body() {
        // TODO: use a uint8_t* and advance it until we reach uint8_t* + body().size()
        // This way, we can avoid indexed access alltogether.
        // For now, we use an indexed access since we are only decoding CONNECT packet
        // for a proof of concept. Then in the future, to support all packet types, we
        // must decode them in a more generic manner.

        // byte 0 : length MSB
        // byte 1 : length LSB
        const uint8_t protocolNameLen = _body[1] - _body[0];
        if (protocolNameLen != 0x4) {
            return reason_code::MALFORMED_PACKET;
        }

        {
            // bytes 2 3 4 5 : MQTT protocol
            const uint8_t protocolOffset = 2;
            //char mqttStr[4];

            // TODO: replace with a string_view
            //std::memcpy(mqttStr, _body.data() + protocolOffset, 4);
            std::string_view mqttStr((char*)_body.data() + protocolOffset, 4);

            // compare non null terminated string
            //if (std::strncmp(mqttStr, "MQTT", 4)) {
            if (mqttStr.compare("MQTT")) {
                // ~~ [MQTT-3.1.2-2]
                return reason_code::UNSUPPORTED_PROTOCOL_VERSION;
            }
        }

        // byte 6 : MQTT version
        if (_body[6] != 0x5) {
            return reason_code::UNSUPPORTED_PROTOCOL_VERSION;
        }

        // Connect flags are only applicable to CONNECT packets. So the idea here is to
        // treat each packet as a connect packet then improve further and further
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
        uint8_t varSize = 0; // length of the variable in the buffer
        // here, we are pretty comfortable that the buffer size is more than 13
        if (utils::decode_variable_int(_body.data() + 10, propertyLength, varSize) != return_code::OK) {
            return reason_code::MALFORMED_PACKET;
        }
        std::cout << "Decoded variable size " << propertyLength << std::endl;

        // decode at position 10 + variable int size + 1
        reason_code rCode;
        rCode = decode_properties(10 + varSize + 1, propertyLength);
        if (rCode != reason_code::SUCCESS) {
            return rCode;
        }
        // TODO: this is ugly and should be removed after testing (use uint8_t* instead)
        uint8_t propertyStart = 10 + varSize + 1;

        // decode payload
        rCode = decode_payload(10 + varSize + 1 + propertyLength);
        if (rCode != reason_code::SUCCESS) {
            return rCode;
        }



        //std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        //std::chrono::system_clock::time_point timeThen;
        //msg >> timeThen;
        //std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";

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

    const reason_code decode_properties(uint8_t start, uint32_t size) {
        // first, check if the _body can hold this data
        if (_body.size() < (start + size)) {
            return reason_code::MALFORMED_PACKET;
        }
        
        // We can use a full c++ implementation here with iterators
        // but we preferred a C-friendly since we are treating a uint8_t
        // buffer

        uint8_t* buff = _body.data() + start;
        const uint8_t* buffEnd = _body.data() + start + size;

        // check if a property type was used twice
        std::unordered_set<property::property_type> propertySet;
        
        // start decoding
        while (buff != buffEnd) { // != or <, which is better?
            
            // We post increment the buffer pointer so we prepare the data reading position right after
            // deducing the property type. Also, this will avoid looping indefinetly since the pointer
            // will increment and leads to an invalid property which itself will early-exit due to 
            // a malformed packet  
            const property::property_type ptype = static_cast<property::property_type>(*(buff++));
            
            // check if this packet type supports this property
            if (!property::types_utils::validate_packet_property_type(ptype, _type)) {
                // check this reason code
                return reason_code::MALFORMED_PACKET;
            }

            // only check if property already exists for unique property types
            if (property::types_utils::is_property_unique(ptype) && !propertySet.insert(ptype).second) {
                return reason_code::PROTOCOL_ERROR;
            }

            // In the following function, the property data is copied from the buffer to a property_data
            // object, which will also be stored in a vector of unique pointers
            uint32_t remainingSize = buffEnd - buff;

            // if remaining size is zero or negative
            if (!remainingSize || (remainingSize > size)) {
                return reason_code::MALFORMED_PACKET;
            }

            reason_code rCode;
            uint32_t propertySize = 0;
            auto propertyDataPtr = property::get_property_data(
                ptype,  // property type: will be used to identify what type of data to extract
                buff,  // the buffer pointer to extract the property data from
                remainingSize,
                propertySize,
                rCode
            );
            
            if (rCode != reason_code::SUCCESS) {
                return rCode;
            }

            // TODO: (only for debugging) Remove this or replace with a trace
            if (ptype == property::property_type::USER_PROPERTY) {
                std::cout << "Displaying property content: \n";
                property::property_data_proxy* data = propertyDataPtr.get();
                property::property_data<std::pair<std::string_view,std::string_view>>* realData = 
                    static_cast<property::property_data<std::pair<std::string_view, std::string_view>>*>(data);
                std::cout << realData->get_data().first << " : " << realData->get_data().second << std::endl;
            }

            _propertyTypes.emplace_back(std::move(propertyDataPtr));

            // prepare the buffer pointer for the next property position
            buff += propertySize;
        }
        return reason_code::SUCCESS;
    }

    // TODO: Change to uint8_t*
    const reason_code decode_payload(uint8_t start) {
        // find a way to check for overflow
        if (_body.size() < start) {
            return reason_code::MALFORMED_PACKET;
        }

        // since the payload is the last part of the body, it is easy to check
        // for out-of-range read
        // TODO: Only temporary, for proof of concept
        uint32_t payloadSize = _body.size() - start;
        uint8_t* buff = _body.data() + start;
        const uint8_t* buffEnd = buff + payloadSize;

        // temporary, for testing purposes
        const payload::payload_type ptype = payload::payload_type::CLIENT_ID;

        uint32_t remainingSize = buffEnd - buff;
        reason_code rCode;
        uint32_t readPayloadSize = 0;
        auto payloadDataPtr = payload::get_payload(
            ptype,  // payload type: will be used to identify what type of data to extract
            buff,  // the buffer pointer to extract the property data from
            remainingSize,
            readPayloadSize,
            rCode
        );

        // TODO: (only for debugging) Remove this or replace with a trace
        if (ptype == payload::payload_type::CLIENT_ID) {
            std::cout << "Displaying payload content: \n";
            payload::payload_proxy* data = payloadDataPtr.get();
            payload::payload<std::string_view>* realData =
                static_cast<payload::payload<std::string_view>*>(data);
            std::cout << " CLIENT_ID : " << realData->get_data() << std::endl;
        }

        return reason_code::SUCCESS;
    }

    friend std::ostream& operator << (std::ostream& os, const packet& packet) {
        os << "PAKCET_TYPE: " << to_string(packet._type) << ", FIXED_HEADER SIZE: " << sizeof(packet._header) << "\n";
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
    std::vector<std::unique_ptr<property::property_data_proxy>> _propertyTypes;
    // maybe use a std::variant in the future
    //std::unordered_map <property::property_type, std::variant<std::string, uint16_t, uint32_t, std::vector<uint8_t>>> _propertyData;
};

} // namespace lmqtt