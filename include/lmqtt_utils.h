#pragma once

#include "lmqtt_common.h"
#include "lmqtt_reason_codes.h"
#include "lmqtt_utf8_string.h"

namespace lmqtt {

class utils {
public:

    // This method takes a buffer, a reference to a decoded value and a reference to an offset.
    // The offset will be used to know from where to start the next reading after
    // decoding the variable.
    static const return_code decode_variable_int(
        uint8_t* buffer,
        uint32_t& decodedValue,
        uint8_t& offset,
        uint32_t buffSize
    ) noexcept {
        decodedValue = 0;
        uint8_t mul = 1;

        for (offset = 0; offset < 4; ++offset) {
            if (offset >= buffSize) {
                return return_code::FAIL;
            }
            decodedValue += (buffer[offset] & 0x7f) * mul;
            if (mul > 0x200000) { // 128 * 128 * 128
                return return_code::FAIL;
            }
            mul *= 0x80; // prepare for next byte
            // no continuation bit, break from the loop
            if (!(buffer[offset] & 0x80)) break;
        }

        return return_code::OK;
    }

    static uint8_t get_variable_int_size(uint32_t value) noexcept {
        if (value >= 0x200000) return 4;
        else if (value >= 0x4000) return 3;
        else if (value >= 0x80) return 2;
        else return 1;
    }

    static const return_code encode_variable_int(
        uint8_t* buffer,
        uint32_t buffSize,
        uint32_t valueToEncode,
        uint8_t& offset
    ) noexcept {
        offset = 0;

        if (buffSize < 4) {
            return return_code::FAIL;
        }

        while (valueToEncode) {
            uint8_t byte = valueToEncode % 0x7f;
            valueToEncode /= 0x7f;

            // if there's more data, set the top bit of this byte
            if (valueToEncode) {
                byte |= 0x7f;
            }

            buffer[offset++] = byte;

            if (offset > 4) {
                return return_code::FAIL;
            }
        }

        return return_code::OK;
    }

    static const return_code decode_utf8_str(uint8_t* buffer,
        std::string_view& decodedString,
        uint32_t& offset,
        bool isAlphaNum = false
    ) noexcept {
        //decodedString.clear();

        uint32_t strLen = (buffer[0] << 0x8) | buffer[1];

        // check for non-null characters first
        // TODO: utf8 string check goes here
        // i is uint32 because we can have a 0xFFFF string so
        // 0xFFFF + 2 (data size) will overflow
        /*for (uint32_t i = 2; i < strLen + 2U; ++i) {
            if (buffer[i] == 0) {
                return return_code::FAIL;
            }
            if (isAlphaNum) {
                if (!isalnum(buffer[i])) {
                    return return_code::FAIL;
                }
            }
        }*/

        decodedString = std::string_view((char*)(buffer + 2), strLen);
        if (!utf8_utils::is_valid_length(decodedString)) {
            return return_code::FAIL;
        }

        if (utf8_utils::is_valid_content(decodedString) != utf8_utils::utf8_str_check::WELL_FORMED) {
            return return_code::FAIL;
        }
        offset += 2 + strLen;

        return return_code::OK;
    }

    static const return_code decode_utf8_str_fixed(uint8_t* buffer,
        std::string_view& decodedString,
        uint32_t length,
        bool isAlphaNum = false
    ) noexcept {

        decodedString = std::string_view((char*)(buffer), length);
        if (!utf8_utils::is_valid_length(decodedString)) {
            return return_code::FAIL;
        }

        if (utf8_utils::is_valid_content(decodedString) != utf8_utils::utf8_str_check::WELL_FORMED) {
            return return_code::FAIL;
        }
        return return_code::OK;
    }

    // a more readable method to convert enums
    template <typename T>
    static constexpr typename std::underlying_type<T>::type to_underlying(T e) {
        return static_cast<typename std::underlying_type<T>::type>(e);
    }
};

} // namespace lmqtt
