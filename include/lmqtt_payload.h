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
    payload(payload_type type, T& data, data_type dtype = data_type::UNKNOWN)
        : payload_proxy(type), _data(data), _dataType(dtype) {}
    payload() = default;
    ~payload() = default;

    T& get_data() noexcept {
        return _data;
    }

    constexpr data_type get_data_type() const {
        return _dataType;
    }

    void set_data(T& data) noexcept {
        _data = data;
    }

    [[nodiscard]] return_code check_data_type(payload_type ptype) {
        if (payload_utils::get_payload_data_type(ptype) != _dataType) {
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
std::unique_ptr<payload_proxy>
get_payload(
    payload_type ptype,
    uint8_t* buff,
    uint32_t remainingSize,
    uint32_t& payloadSize,
    reason_code& rCode
) noexcept {

    rCode = reason_code::SUCCESS;

    switch (payload_utils::get_payload_data_type(ptype)) {
    case data_type::UTF8_STRING:
    case data_type::UTF8_STRING_ALPHA_NUM:
    {
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

        bool isAlphaNumStr = payload_utils::get_payload_data_type(ptype) == data_type::UTF8_STRING_ALPHA_NUM;

        if (utils::decode_utf8_str(buff, str, offset, isAlphaNumStr) != return_code::OK) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<payload_proxy>{};
        }

        payloadSize = offset;
        std::unique_ptr<payload_proxy> payloadData(
            new payload<std::string_view>(ptype, str, (isAlphaNumStr)? data_type::UTF8_STRING_ALPHA_NUM : data_type::UTF8_STRING)
        );
        return payloadData;
    }
    case data_type::BINARY:
    {
        // binary data should be accessed as a view too, to avoid duplicating data
        // into memory
        if (remainingSize < 2) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<payload_proxy>{};
        }

        uint16_t dataLen = (buff[0] << 0x8) | buff[1];

        if (remainingSize < (2U + dataLen)) {
            rCode = reason_code::MALFORMED_PACKET;
            return std::unique_ptr<payload_proxy>{};
        }

        std::vector<uint8_t> data;
        data.assign(buff + 2, buff + 2 + dataLen);
        std::unique_ptr<payload_proxy> payloadData(
            new payload<std::vector<uint8_t>>(ptype, data, data_type::BINARY)
        );

        payloadSize = dataLen + 2;
        return payloadData;
    }
    default:
        payloadSize = 0;
        rCode = reason_code::MALFORMED_PACKET;
        return std::unique_ptr<payload_proxy>{};
    }
}

} //namespace property

} // namespace lmqtt
