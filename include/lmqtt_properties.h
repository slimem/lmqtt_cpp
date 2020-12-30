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
    property_data(property_type type, T& data, data_type dtype = data_type::UNKNOWN)
        : property_data_proxy(type), _data(data), _dataType(dtype) {}
    property_data() = default;
    ~property_data() = default;
    T& get_data() noexcept {
        return _data;
    }

    constexpr data_type get_data_type() const {
        return _dataType;
    }
    void set_data(T& data) noexcept {
        _data = data;
    }

    [[nodiscard]] return_code check_data_type(property_type ptype) {
        if (types_utils::get_property_data_type(ptype) != _dataType) {
            return return_code::FAIL;
        }
        return return_code::OK;
    }
private:
    T _data;
    data_type _dataType;
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
            new property_data<uint8_t>(ptype, data, data_type::BYTE)
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
            new property_data<uint16_t>(ptype, data, data_type::TWO_BYTES_INT)
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
            new property_data<uint32_t>(ptype, data, data_type::FOUR_BYTES_INT)
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
            new property_data<std::string_view>(ptype, str, data_type::UTF8_STRING)
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
            new property_data<std::pair<std::string_view, std::string_view>>(
                ptype,
                strPair,
                data_type::UTF8_STRING_PAIR
                )
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
            new property_data<std::vector<uint8_t>>(ptype, data, data_type::BINARY)
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

template<typename T>
[[nodiscard]] return_code write_property_to_buffer(uint8_t* buffer, uint32_t buffSize, T data) {
    std::cout << "[WARNING] -- Writing unknown property data type to buffer: " << typeid(T).name();
    return return_code::FAIL;
}

template<>
[[nodiscard]] return_code write_property_to_buffer<uint8_t>(uint8_t* buffer, uint32_t buffSize, uint8_t data) {

    if (buffSize < sizeof(uint8_t)) {
        //TODO: add more meningful debug message
        return return_code::FAIL;
    }

    *buffer = data;
    return return_code::OK;
}

template<>
[[nodiscard]] return_code write_property_to_buffer<uint16_t>(uint8_t* buffer, uint32_t buffSize, uint16_t data) {

    if (buffSize < sizeof(uint16_t)) {
        //TODO: add more meningful debug message
        return return_code::FAIL;
    }

    buffer[0] = data >> 0x8;
    buffer[1] = data & 0xFF;

    return return_code::OK;
}

template<>
[[nodiscard]] return_code write_property_to_buffer<uint32_t>(uint8_t* buffer, uint32_t buffSize, uint32_t data) {

    if (buffSize < sizeof(uint32_t)) {
        //TODO: add more meningful debug message
        return return_code::FAIL;
    }

    buffer[0] = data >> 0x18;
    buffer[1] = (data >> 0x10) & 0xFF;
    buffer[2] = (data >> 0x8) & 0xFF;
    buffer[3] = data & 0xFF;

    return return_code::OK;
}

template<>
[[nodiscard]] return_code write_property_to_buffer<std::string&>(uint8_t* buffer, uint32_t buffSize, std::string& data) {

    if (buffSize < (data.size() + 2)) {
        //TODO: add more meningful debug message
        return return_code::FAIL;
    }

    uint16_t strSize = (uint16_t)data.size();
    buffer[0] = strSize >> 0x8;
    buffer[1] = strSize & 0xFF;

    std::memcpy(buffer + 2, data.c_str(), strSize);

    return return_code::OK;
}

template<>
[[nodiscard]] return_code write_property_to_buffer<std::pair<const std::string, const std::string>&>(
    uint8_t* buffer,
    uint32_t buffSize,
    std::pair<const std::string, const std::string>& data) {

    uint16_t str1Size = (uint16_t)data.first.size();
    uint16_t str2Size = (uint16_t)data.second.size();
    uint32_t str2WritePos = str1Size + 4;

    uint32_t buffSizeCompare = str1Size + str2Size + 4;

    if (buffSize < buffSizeCompare) {
        //TODO: add more meningful debug message
        return return_code::FAIL;
    }

    buffer[0] = str1Size >> 0x8;
    buffer[1] = str1Size & 0xFF;
    std::memcpy(buffer + 2, data.first.c_str(), str1Size);

    buffer[2 + str1Size]        = str2Size >> 0x8;
    buffer[2 + str1Size + 1]    = str2Size & 0xFF;
    std::memcpy(buffer + 2 + str1Size + 2, data.second.c_str(), str2Size);

    return return_code::OK;
}

template<>
[[nodiscard]] return_code write_property_to_buffer<std::vector<uint8_t>&>(
    uint8_t* buffer,
    uint32_t buffSize,
    std::vector<uint8_t>& data) {

    uint16_t dataSize = (uint16_t)data.size();

    uint32_t buffSizeCompare = dataSize + 2;

    if (buffSize < buffSizeCompare) {
        //TODO: add more meningful debug message
        return return_code::FAIL;
    }

    buffer[0] = dataSize >> 0x8;
    buffer[1] = dataSize & 0xFF;
    std::memcpy(buffer + 2, data.data(), dataSize);

    return return_code::OK;
}

} //namespace property

} // namespace lmqtt
