#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"
#include "lmqtt_utils.h"

namespace lmqtt {

namespace payload {

class payload_proxy {
public:
    payload_proxy()
        : _payloadType(payload_type::UNKNOWN) {}
    payload_proxy(payload_type type)
        : _payloadType(type) {}
    ~payload_proxy() = default;
    const payload_type get_payload_type() const noexcept {
        return _payloadType;
    }
    void set_payload_type(payload_type type) noexcept {
        _payloadType = type;
    }
private:
    payload_type _payloadType;
};

template <typename T>
class payload : public payload_proxy {
public:
    payload(payload_type type, T& data) : payload_proxy(type), _data(data) {}
    payload() = default;
    ~payload() = default;
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
std::unique_ptr<payload_proxy>
get_payload(
    payload_type ptype,
    uint8_t* buff,
    uint32_t remainingSize,
    uint32_t& payloadSize,
    reason_code& rCode
) noexcept {

    rCode = reason_code::SUCCESS;

    std::string_view str;
    payloadSize = 0;
    uint32_t offset = 0;

    if (remainingSize < 2U) {
        rCode = reason_code::MALFORMED_PACKET;
        return std::unique_ptr<payload_proxy>{};
    }
    uint16_t strLen = (buff[0] << 0x8) | buff[1];

    if (remainingSize < (2U + strLen)) {
        rCode = reason_code::MALFORMED_PACKET;
        return std::unique_ptr<payload_proxy>{};
    }

    if (utils::decode_utf8_str(buff, str, offset) != return_code::OK) {
        rCode = reason_code::MALFORMED_PACKET;
        return std::unique_ptr<payload_proxy>{};
    }

    std::unique_ptr<payload_proxy> propertyData(
        new payload<std::string_view>(ptype, str)
    );
    return propertyData;

}

} //namespace property

} // namespace lmqtt
