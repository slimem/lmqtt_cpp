#pragma once

#include "lmqtt_common.h"
#include "lmqtt_reason_codes.h"
#include "lmqtt_types.h"
#include "lmqtt_properties.h"
#include "lmqtt_payload.h"
#include "lmqtt_utils.h"
#include "lmqtt_client_config.h"

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
    constexpr void reset() noexcept {
        _controlField = 0;
        _packetLen = 0;
    }
};

/*
 * Fixed Header: CONNACK
 * Fixed Header + Variable Header: PUBACK
 * Fixed Header + Variable Header + Payload: CONNECT
 */
class lmqtt_packet {
    friend class connection;

    fixed_header _header {};
    // the following vector is used for the following:
    // 1 - For received packets, it holds the packet except the
    //     fixed header
    // 2 - For to-be-sent packets, it holds everything. This will
    //     avoid using a different container for sent packets
    std::vector<uint8_t> _body;
    packet_type _type = packet_type::UNKNOWN;

    void reset() noexcept {
        _header.reset();
        _type = packet_type::UNKNOWN;
        std::memset(_body.data(), 0, _body.size());
        std::memset(_varIntBuff, 0, 4);
    }
    
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
            return reason_code::SUCCESS;
            }
            break;
        case packet_type::CONNACK:
            {
                _type = packet_type::CONNACK;
                // check for field
                if ((uint8_t)packet_flag::CONNACK != pflag) {
                    return reason_code::MALFORMED_PACKET;
                }
                return reason_code::SUCCESS;
                break;
            }
            
        case packet_type::PUBLISH:
        {
            _type = packet_type::PUBLISH;

            uint8_t dub = pflag >> 3;
            uint8_t qosLevel = (pflag >> 1) & 0x3;
            uint8_t retain = pflag & 0x1;

            if (qosLevel || dub) {
                return reason_code::MALFORMED_PACKET;
            }

            return reason_code::SUCCESS;
            break;
        }
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
        {
            if ((uint8_t)packet_flag::DISCONNECT != pflag) {
                return reason_code::MALFORMED_PACKET;
            }
            _type = packet_type::DISCONNECT;
            return reason_code::SUCCESS;
            break;
        }
        case packet_type::AUTH:
            break;
        default:
            {
                // malformet packet
                return reason_code::MALFORMED_PACKET;
            }
            break;
        }
        return reason_code::MALFORMED_PACKET;
    }

    [[nodiscard]] const reason_code decode_packet_body() {
        // for now, only decode CONNECT packet
        switch (_type) {
        case packet_type::CONNECT:
            return decode_connect_packet_body();
        }
        return reason_code::SUCCESS;
    }

    [[nodiscard]] const reason_code decode_connect_packet_body() {
        std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();
        // TODO: use a uint8_t* and advance it until we reach uint8_t* + body().size()
        // This way, we can avoid indexed access alltogether.
        // For now, we use an indexed access since we are only decoding CONNECT packet
        // for a proof of concept. Then in the future, to support all packet types, we
        // must decode them in a more generic manner.

        uint8_t* ptr = _body.data();

        // byte 0 : length MSB
        // byte 1 : length LSB
        const uint8_t protocolNameLen = *(ptr + 1) + *ptr;
        if (protocolNameLen != 0x4) {
            return reason_code::MALFORMED_PACKET;
        }

        {
            // bytes 2 3 4 5 : MQTT protocol
            const uint8_t protocolOffset = 2;
            //char mqttStr[4];

            std::string_view mqttStr((char*) ptr + protocolOffset, protocolNameLen);

            // compare non null terminated string
            //if (std::strncmp(mqttStr, "MQTT", 4)) {
            if (mqttStr.compare("MQTT")) {
                // ~~ [MQTT-3.1.2-1]
                return reason_code::UNSUPPORTED_PROTOCOL_VERSION;
            }
        }

        {
            // byte 6 : MQTT version
            const uint8_t mqttVersionOffset = 6;
            if (*(ptr + mqttVersionOffset) != 0x5) {
                // ~~ [MQTT-3.1.2-2]
                return reason_code::UNSUPPORTED_PROTOCOL_VERSION;
            }
        }

        // Connect flags are only applicable to CONNECT packets. So the idea here is to
        // treat each packet as a connect packet then improve further and further
        // byte 7 : Connect flags
        {
            const uint8_t flagOffset = 7;
            const uint8_t flags = *(ptr + flagOffset);
            // bit 0 : Reserved : first check the reserved flag is set to 0
            // ~~ [MQTT-3.1.2-3]
            if (flags & 0x1) {
                return reason_code::MALFORMED_PACKET;
            }

            // Extract the reset of the flags with bitmasking
            //TODO: Replace with bitfield in the future
            // bit 1 : Clean start
            _clientCfg->_cleanStart = (flags & 0x2) >> 1;

            {
                // bit 2 : Will Flag
                _clientCfg->_willFlag = (flags & 0x4) >> 2;
                
                if (_clientCfg->_willFlag) {
                    _payloadFlags[utils::to_underlying(payload::payload_type::WILL_PROPERTIES)] = payload::payload_type::WILL_PROPERTIES;
                    _payloadFlags[utils::to_underlying(payload::payload_type::WILL_TOPIC)]      = payload::payload_type::WILL_TOPIC;
                    _payloadFlags[utils::to_underlying(payload::payload_type::WILL_PAYLOAD)]    = payload::payload_type::WILL_PAYLOAD;
                    
                    // bit 3 and 4 : Will QoS
                    _clientCfg->_willQos = (flags & 0x18) >> 3;
                    
                    // check if willQos is valid
                    if (_clientCfg->_willQos == 0x3) {
                        return reason_code::MALFORMED_PACKET;
                    }
                    // init the will config unique ptr. In the future, if the will ptr is still null,
                    // it is a malformted packet
                    _clientCfg->init_will_cfg();
                } else {
                    _clientCfg->_willQos = 0;
                }

                // TODO: Use in the future
                // bit 5 : Will retain
                _clientCfg->_willRetain = (flags & 0x20) >> 5;
            }

            // bit 6 : Password Flag
            _clientCfg->_passwordFlag = (flags & 0x40) >> 6;
            if (_clientCfg->_passwordFlag) {
                _payloadFlags[utils::to_underlying(payload::payload_type::PASSWORD)] = payload::payload_type::PASSWORD;
            }
            // bit 7 : User name flag
            _clientCfg->_userNameFlag = (flags & 0x80) >> 7;
            if (_clientCfg->_userNameFlag) {
                _payloadFlags[utils::to_underlying(payload::payload_type::USER_NAME)] = payload::payload_type::USER_NAME;
            }
        }

        {
            // byte 8 and 9 : Keep alive MSB and LSB
            const uint8_t keepAliveSize = 2;
            const uint8_t keepAliveOffset = 8;
            _clientCfg->_keepAlive = (*(ptr + keepAliveOffset) << 8) | *(ptr + keepAliveOffset + 1);
        }

        // check if body size can hold a maximum variable length int (base + 3)
        if (_body.size() < 13) { // starts at 10 and ends at 13
            return reason_code::MALFORMED_PACKET;
        }

        // now compute the variable
        uint32_t propertyLength = 0;
        uint8_t varSize = 0; // length of the variable in the buffer 
        // here, we are pretty comfortable that the buffer size is more than 13
        if (utils::decode_variable_int(_body.data() + 10, propertyLength, varSize, _body.size() - 10) != return_code::OK) {
            return reason_code::MALFORMED_PACKET;
        }

        // decode at position 10 + variable int size + 1
        reason_code rCode;
        // TODO: should be buffer indexed
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

        std::chrono::system_clock::time_point timeEnd = std::chrono::system_clock::now();
        //std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        //std::chrono::system_clock::time_point timeThen;
        //msg >> timeThen;
        //std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
        std::cout << "[DEBUG] -- FINISHED PARSING CONNACK PACKET (TOOK " << std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count() << "us)\n";

        return reason_code::SUCCESS;
    }

    [[nodiscard]] const reason_code decode_publish_packet_body() {
        std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();

        auto it = _body.begin();

        // Variable header fields in order: Topic name, packet id, properties

        // Topic Name
        std::string_view topic;
        uint32_t offset = 0;
        utils::decode_utf8_str(&(*it), topic, offset);
        _clientCfg->_lastTopic = topic;

        it += offset;

        // now compute the variable
        uint32_t propertyLength = 0;
        uint8_t varSize = 0; // length of the variable in the buffer 
        // here, we are pretty comfortable that the buffer size is more than 13
        if (utils::decode_variable_int(&(*it), propertyLength, varSize, std::distance(it, _body.end())) != return_code::OK) {
            return reason_code::MALFORMED_PACKET;
        }

        it += varSize;
        uint32_t propertyStart = std::distance(_body.begin(), it);
        reason_code rcode = decode_properties(propertyStart, propertyLength);

        it++;

        std::string_view message;
        uint32_t messageLength = std::distance(it, _body.end());
        //uint32_t messageOffset = 0;
        if (messageLength == 0 || messageLength > 0xFF) {
            return reason_code::MALFORMED_PACKET;
        }
        
        if (utils::decode_utf8_str_fixed(&(*it), message, messageLength) != return_code::OK) {
            return reason_code::MALFORMED_PACKET;
        }
        
        if (utf8_utils::has_wildcard(message)) {
            return reason_code::MALFORMED_PACKET;
        }

        //std::cout << "[" << _clientCfg->_clientId << "] " << _clientCfg->_lastTopic << " : " << message << std::endl;

        std::chrono::system_clock::time_point timeEnd = std::chrono::system_clock::now();
        std::cout << "[DEBUG] -- FINISHED PARSING PUBLISH PACKET (TOOK " << std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count() << "us)\n";
        return reason_code::SUCCESS;
    }

    [[nodiscard]] const reason_code decode_disconnect_packet_body() {
        std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();

        if (_body.empty()) {
            return reason_code::SUCCESS;
        }

        auto it = _body.begin();

        reason_code dReasonCode = static_cast<reason_code>(*it);

        if (dReasonCode != reason_code::SUCCESS) {
            // TODO: Add text
            std::cout << "[SERVER] - Client disconnected with a reason code " << std::hex << static_cast<uint8_t>(dReasonCode) << std::endl;
            return reason_code::SUCCESS;
        }

        std::chrono::system_clock::time_point timeEnd = std::chrono::system_clock::now();
        std::cout << "[DEBUG] -- FINISHED PARSING DISCONNECT PACKET (TOOK " << std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count() << "us)\n";
        return reason_code::SUCCESS;
    }

    const reason_code decode_properties(uint32_t start, uint32_t size, bool isWillProperties = false) {
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
                property::property_data_proxy* data = propertyDataPtr.get();
                property::property_data<std::pair<std::string_view,std::string_view>>* realData = 
                    static_cast<property::property_data<std::pair<std::string_view, std::string_view>>*>(data);
                //std::cout << realData->get_data().first << " : " << realData->get_data().second << std::endl;
            }

            reason_code rcode;
            if (isWillProperties) {
                rcode = _clientCfg->configure_will_propriety(std::move(propertyDataPtr));
            } else {
                rcode = _clientCfg->configure_propriety(std::move(propertyDataPtr));
            }
            if (rcode != reason_code::SUCCESS) {
                return rcode;
            }
            //_propertyTypes.emplace_back(std::move(propertyDataPtr));

            // prepare the buffer pointer for the next property position
            buff += propertySize;
        }
        return reason_code::SUCCESS;
    }

    // TODO: Change to uint8_t*
    const reason_code decode_payload(uint32_t start) {
        // find a way to check for overflow
        if (_body.size() < start) {
            return reason_code::MALFORMED_PACKET;
        }

        // since the payload is the last part of the body, it is easy to check
        // for out-of-range read
        // TODO: Only temporary, for proof of concept
        uint32_t totalPayloadSize = _body.size() - start;
        uint8_t* buff = _body.data() + start;
        const uint8_t* buffEnd = buff + totalPayloadSize;

        for (const auto ptype : _payloadFlags) {

            if (buff == buffEnd) {
                // finally
                return reason_code::SUCCESS;
            }

            if (buff > buffEnd) {
                return reason_code::MALFORMED_PACKET;
            }

            if (ptype == payload::payload_type::UNKNOWN) continue;


            uint32_t remainingSize = buffEnd - buff;
            reason_code rCode;
            uint32_t readPayloadSize = 0;

            if (ptype == payload::payload_type::WILL_PROPERTIES) {
                uint32_t willPropertyLength = 0;
                uint8_t varSize = 0;
                if (utils::decode_variable_int(buff, willPropertyLength, varSize, remainingSize) != return_code::OK) {
                    return reason_code::MALFORMED_PACKET;
                }

                buff += varSize;
                reason_code rCode;
                rCode = decode_properties(buff - _body.data(), willPropertyLength, true);
                if (rCode != reason_code::SUCCESS) {
                    return rCode;
                }

                buff += willPropertyLength;
            } else {

                auto payloadDataPtr = payload::get_payload(
                    ptype,  // payload type: will be used to identify what type of data to extract
                    buff,  // the buffer pointer to extract the property data from
                    remainingSize,
                    readPayloadSize,
                    rCode
                );

                if (rCode != reason_code::SUCCESS) {
                    return rCode;
                }

                // move the buffer pointer to the next read
                buff += readPayloadSize;

                // TODO: (only for debugging) Remove this or replace with a trace
                if (ptype == payload::payload_type::CLIENT_ID) {
                    payload::payload_proxy* data = payloadDataPtr.get();
                    payload::payload<std::string_view>* realData =
                        static_cast<payload::payload<std::string_view>*>(data);
                    //std::cout << "CLIENT_ID : " << realData->get_data() << std::endl;
                }
                
                reason_code rcode = _clientCfg->configure_payload(std::move(payloadDataPtr));
                if (rcode != reason_code::SUCCESS) {
                    return rcode;
                }
            }
        }

        return reason_code::SUCCESS;
    }

    friend std::ostream& operator << (std::ostream& os, const lmqtt_packet& packet) {
        os << "PACKET_TYPE: " << to_string(packet._type) << ", FIXED_HEADER SIZE: " << sizeof(packet._header) << "\n";
        return os;
    }

public:
    size_t size() const noexcept {
        return _header.size() + _body.size();
    }

public:
    
    [[nodiscard]] return_code create_connack_packet(
        packet_type packetType,
        reason_code reasonCode
    ) {
        if (!packet::utils::is_server_packet(packetType)) {
            return return_code::FAIL;
        }

        // The connack packet size is the following:
        // 1 byte + (1 to 4) bytes + 1 byte + 1 byte + N bytes (properties size)
        
        // In the variable header, we only know the size of the following:
        // 1 byte (Acknowledge flags) + 1 byte (Reason code) = 2
        uint32_t variabePacketSize = 2;
        uint32_t propertiesSize = 0;
        
        // first we must pre-compute the size of all properties
        uint8_t propertiesVarSize = 0;
        {
            for (auto ptype : property::connack_properties) {
                propertiesSize += _clientCfg->get_property_size(ptype);
            }
            propertiesVarSize = utils::get_variable_int_size(propertiesSize);

        }
        variabePacketSize += (propertiesSize + propertiesVarSize);

        // now we compute the size of the variable size int (1 to 4 bytes)
        uint8_t variableIntSize = utils::get_variable_int_size(variabePacketSize);
        
        uint32_t fullPacketSize = variableIntSize + variabePacketSize;

        fullPacketSize++; // we add the first byte of the header 

        _body.resize(fullPacketSize);

        // We start filling our packet body

        // Fixed header
        {
            _body[0] = static_cast<uint8_t>(packetType) << 4;
            uint8_t viSize;
            if (utils::encode_variable_int(_body.data() + 1, _body.size() - 1, variabePacketSize, viSize) != return_code::OK) {
                return return_code::FAIL;
            }
            if (viSize != utils::get_variable_int_size(variabePacketSize)) {
                return return_code::FAIL;
            }
        }

        // Variable header
        
        uint32_t variableHeaderStart = 1 + variableIntSize;

        // Connect acknowledge flags
        // if clean start is 1; this value must be 0
        _body[variableHeaderStart + 0] = 0;

        // reason code
        _body[variableHeaderStart + 1] = static_cast<uint8_t>(reasonCode);

        // Connack properties
        // Encode properties variable int size
        {
            uint8_t viSize;
            if (utils::encode_variable_int(_body.data() + variableHeaderStart + 2, _body.size() - 1, propertiesSize, viSize) != return_code::OK) {
                return return_code::FAIL;
            }
            if (viSize != propertiesVarSize) {
                return return_code::FAIL;
            }
        }



        uint8_t* buff = _body.data() + variableHeaderStart + 2 + propertiesVarSize;
        const uint8_t* buffEnd = _body.data() + _body.size();
        uint32_t index = 0;
        uint32_t remainingSize = _body.size() - (variableHeaderStart + 2 + propertiesVarSize);
        if (remainingSize != propertiesSize) {
            return return_code::FAIL;
        }
        while ((buff != buffEnd)) {

            auto ptype = property::connack_properties[index++];
            //while (buff != buffEnd) {
            //
            //}
            //buff[index++] = static_cast<uint8_t>(ptype);

            uint32_t propertySize = 0;
            if (_clientCfg->fill_property(buff, remainingSize, ptype, propertySize) != return_code::OK) {
                return return_code::FAIL;
            }
            buff += propertySize;
            remainingSize -= propertySize;
        }
        //for (auto ptype : property::connack_properties) {
        //    _clientCfg->fill_property(ptype);
        //}

        /*encode_variable_int(
            uint8_t * buffer,
            uint32_t valueToEncode,
            uint8_t & offset,
            uint32_t buffSize*/

        return return_code::OK;
    }

    return_code create_short_packet() {
        _body.resize(4);

        _body[0] = 2 << 4;
        _body[1] = 2;
        _body[2] = 0;
        _body[3] = static_cast<uint8_t>(reason_code::SUCCESS);
        return return_code::OK;
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
        case packet_type::PUBLISH:          return _clientCfg->_qos > 0 ? true : false; // true of QoS > 0
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

    std::shared_ptr<client_config> _clientCfg;

    uint8_t _varIntBuff[4]; // a buffer to decode variable int

protected:

    // order is important and the maximum number of payloads is known so use a container
    // at compile time
    std::array<payload::payload_type, 6> _payloadFlags{
        payload::payload_type::CLIENT_ID,
        payload::payload_type::UNKNOWN,
        payload::payload_type::UNKNOWN,
        payload::payload_type::UNKNOWN,
        payload::payload_type::UNKNOWN,
        payload::payload_type::UNKNOWN
    };
};

} // namespace lmqtt
