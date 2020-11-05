#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"
#include "lmqtt_utils.h"

namespace lmqtt {

namespace property {

class property_data_proxy {
public:
    property_data_proxy() 
        : _propertyType(property_type::UNKNOWN) {}
    property_data_proxy(property_type type)
        : _propertyType(type) {}
    ~property_data_proxy() = default;
    const property_type get_property_type() const noexcept {
        return _propertyType;
    }
    void set_property_type(property_type type) noexcept {
        _propertyType = type;
    }
private:
    property_type _propertyType;
};

template <typename T>
class property_data : public property_data_proxy {
public:
    property_data(property_type type, T& data) : property_data_proxy(type), _data(data) {}
    property_data() = default;
    ~property_data() = default;
    const T& get_data() const noexcept {
        return _data;
    }
    void set_data(T& data) noexcept {
        _data = data;
    }
private:
    T _data;
};

// Small data (data smaller than uint32_t is copied, big data (buffers, strings)
// are indexed in the buffer directly. For strings, string_view does only that.
// we will implement the same concept for raw data (something named data_view)
// TODO
std::unique_ptr<property_data_proxy>
get_property_data(
    property_type ptype,
    uint8_t* buff,
    uint32_t remainingSize,
    uint32_t& propertySize,
    reason_code& rCode
) noexcept {

    rCode = reason_code::SUCCESS;

    switch (types_utils::get_property_data_type(ptype)) {
    case data_type::BYTE:
    {
        // if no remaining size, we early exit
        if (!remainingSize) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }
        uint8_t data = buff[0];
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<uint8_t>(ptype, data)
        );
        propertySize = 1;
        return propertyData;
    }
    case data_type::TWO_BYTES_INT:
    {
        if (remainingSize < 2) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }
        uint16_t data = (buff[0] << 0x8) | buff[1];
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<uint16_t>(ptype, data)
        );
        propertySize = 2;
        return propertyData;
    }
    case data_type::FOUR_BYTES_INT:
    {
        if (remainingSize < 4) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }
        uint32_t data = 
            (buff[0] << 0x18) |
            (buff[1] << 0x10) |
            (buff[2] << 0x8) |
            buff[3];
        propertySize = 4;
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<uint32_t>(ptype, data)
        );
        return propertyData;
    }
    case data_type::VARIABLE_BYTE_INT:
    {
        propertySize = 4;
        std::cout << "NOT SUPPORTED YET\n";
        rCode = reason_code::MALFORMED_PACKET;
        return std::unique_ptr<property_data_proxy>{};
    }
    case data_type::UTF8_STRING:
    {
        std::string_view str;
        uint32_t offset = 0;

        if (remainingSize < 2U) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }
        uint16_t strLen = (buff[0] << 0x8) | buff[1];

        if (remainingSize < (2U + strLen)) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        if (utils::decode_utf8_str(buff, str, offset) != return_code::OK) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }
        propertySize = offset;
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<std::string_view>(ptype, str)
        );
        return propertyData;
    }
    case data_type::UTF8_STRING_PAIR:
    {
        std::pair<std::string_view, std::string_view> strPair;
        uint32_t offset = 0;
        
        // ******** BOUNDARY CHECK FOR FIRST STRING ***** //
        // first, check for str size
        if (remainingSize < 2) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }
        uint16_t str1Len = (buff[0] << 0x8) | buff[1];

        // if the remaining size is less than the str length + 2, the extracted
        // string will be out-of-range
        if (remainingSize < (2U + str1Len)) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        if (utils::decode_utf8_str(buff, strPair.first, offset) != return_code::OK) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        // ******** BOUNDARY CHECK FOR SECOND STRING ***** //
        // first, check for str size
        if (remainingSize < (offset + 2U)) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        uint16_t str2Len = (buff[offset] << 0x8) | buff[offset + 1];
        
        // if the remaining size is less than the two string lenght + 4, the extracted
        // string will be out-of-range
        if (remainingSize < (4U + str1Len + str2Len)) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        if (utils::decode_utf8_str(buff + offset, strPair.second, offset) != return_code::OK) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        // the next property will be parsed from the new position
        propertySize = offset;
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<std::pair<std::string_view, std::string_view>>(ptype, strPair)
        );
        return propertyData;
    }
    case data_type::BINARY:
    {
        // binary data should be accessed as a view too, to avoid duplicating data
        // into memory
        if (remainingSize < 2) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        uint16_t dataLen = (buff[0] << 0x8) | buff[1];

        if (remainingSize < (2U + dataLen)) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<property_data_proxy>{};
        }

        std::vector<uint8_t> data;
        data.assign(buff + 2, buff + 2 + dataLen);
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<std::vector<uint8_t>>(ptype, data)
        );
        propertySize = dataLen + 2;
        return propertyData;
    }
    default:
        propertySize = 0;
        rCode = reason_code::MALFORMED_PACKET;
        return std::unique_ptr<property_data_proxy>{};
    }    
}

} //namespace property

} // namespace lmqtt
