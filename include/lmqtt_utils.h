#pragma once

#include "lmqtt_common.h"
#include "lmqtt_reason_codes.h"

namespace lmqtt {

class utils {
public:

    // This method takes a buffer, a reference to a decoded value and a reference to an offset.
    // The offset will be used to know from where to start the next reading after
    // decoding the variable.
    static const return_code decode_variable_int(
        uint8_t* buffer,
        uint32_t& decodedValue,
        uint8_t& offset
    ) noexcept {
        decodedValue = 0;
        uint8_t mul = 1;

        for (offset = 0; offset < 4; ++offset) {
            decodedValue += buffer[offset] & 0x7f * mul;
            if (mul > 0x200000) { // 128 * 128 * 128
                return return_code::FAIL;
            }
            mul *= 0x80; // prepare for next byte
            // no continuation bit, break from the loop
            if (!(buffer[offset] & 0x80)) break;
        }

        return return_code::OK;
    }

    static const return_code decode_utf8_str(uint8_t* buffer,
        std::string_view& decodedString,
        uint32_t& offset
    ) noexcept {
        //decodedString.clear();

        uint16_t strLen = (buffer[0] << 0x8) | buffer[1];

        // check for non-null characters first
        // TODO: utf8 string check goes here
        // i is uint32 because we can have a 0xFFFF string so
        // 0xFFFF + 2 (data size) will overflow
        for (uint32_t i = 2; i < strLen + 2U; ++i) {
            if (buffer[i] == 0) {
                return return_code::FAIL;
            }
        }

        decodedString = std::string_view((char*)(buffer + 2), strLen);
        offset += 2 + strLen;

        return return_code::OK;
    }

    // a more readable method to convert enums
    template <typename T>
    static constexpr typename std::underlying_type<T>::type to_underlying(T e) {
        return static_cast<typename std::underlying_type<T>::type>(e);
    }
};

} // namespace lmqtt
