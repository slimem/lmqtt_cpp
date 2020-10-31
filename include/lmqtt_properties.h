#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"

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

std::unique_ptr<property_data_proxy>
get_property_data(property_type ptype, uint8_t* buff, uint32_t buffLength, reason_code& reasonCode) noexcept {
    reasonCode = reason_code::SUCCESS;
    /*std::unique_ptr<property::property_data_proxy> propertyData(
    new property::property_data<std::string>(
        property::property_type::CONTENT_TYPE, potato1)
);*/

    switch (utils::get_property_data_type(ptype)) {
    case data_type::BYTE:
        return std::unique_ptr<property_data_proxy>{};
    case data_type::TWO_BYTES_INT:
    {
        uint16_t data = (buff[0] << 0x8) | buff[1];
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<uint16_t>(ptype, data)
        );
        return propertyData;
    }
    case data_type::FOUR_BYTES_INT:
    {
        uint32_t data = (buff[0] << 0x18) |
            (buff[1] << 0x10) |
            (buff[2] << 0x8) |
            buff[3];
        std::unique_ptr<property_data_proxy> propertyData(
            new property_data<uint32_t>(ptype, data)
        );
        return propertyData;
    }
    case data_type::VARIABLE_BYTE_INT:
        return std::unique_ptr<property_data_proxy>{};
    case data_type::UTF8_STRING:
        return std::unique_ptr<property_data_proxy>{};
    case data_type::UTF8_STRING_PAIR:
        return std::unique_ptr<property_data_proxy>{};
    case data_type::BINARY:
        return std::unique_ptr<property_data_proxy>{};
    case data_type::UNKNOWN:
        return std::unique_ptr<property_data_proxy>{};
    default:
        return std::unique_ptr<property_data_proxy>{};
    }
    return std::unique_ptr<property_data_proxy>{};
    
}

} //namespace property

} // namespace lmqtt
