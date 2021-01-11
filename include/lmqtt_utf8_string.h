#pragma once
#include "lmqtt_common.h"

namespace lmqtt {

class utf8_utils {
public:

    enum class utf8_str_check : uint8_t {
        WELL_FORMED                 = 0,
        WELL_FORMED_NO_CHARACTOR    = 1,
        ILL_FORMED                  = 2,
    };

    static constexpr utf8_str_check is_valid_content(std::string_view str) {
        auto it = std::begin(str);
        auto end = std::end(str);
        auto result = utf8_str_check::WELL_FORMED;

        while (it != end) {
            uint8_t byte1 = static_cast<uint8_t>(*it);
            if (byte1 < 0b1000'0000) {
                // 0xxx'xxxx (7 bit ascii character)
                if (byte1 == 0x00) {
                    // null character
                    return utf8_str_check::ILL_FORMED;
                }
                if ((byte1 >= 0x01 && byte1 <= 0x1f)
                    || byte1 == 0x7f) {
                    result = utf8_str_check::WELL_FORMED_NO_CHARACTOR;
                }
                ++it;
            } else if ((byte1 & 0b1110'0000) == 0b1100'0000) {
                // 110x'xxxx 10xx'xxxx
                if ((it + 1) == end) {
                    return utf8_str_check::ILL_FORMED;
                }

                uint8_t byte2 = static_cast<uint8_t>(*(it + 1));
                if ((byte2 & 0b1100'0000) != 0b1000'0000
                    || (byte1 & 0b1111'1110) == 0b1100'0000) {
                    // overlong
                    return utf8_str_check::ILL_FORMED;
                }
                if (byte1 == 0b1100'0010
                    && byte1 >= 0b1000'0000
                    && byte1 <= 0b1001'1111) {
                    result = utf8_str_check::WELL_FORMED_NO_CHARACTOR;
                }
                it += 2;
            } else if ((byte1 & 0b1111'0000) == 0b1110'0000) {
                // 1110'xxxx 10xx'xxxx 10xx'xxxx
                if ((it + 2) == end) {
                    return utf8_str_check::ILL_FORMED;
                }
                uint8_t byte2 = static_cast<uint8_t>(*(it + 1));
                uint8_t byte3 = static_cast<uint8_t>(*(it + 2));
                if ((byte2 & 0b1100'0000) != 0b1000'0000
                    || (byte3 & 0b1100'0000) != 0b1000'0000
                    || (byte1 == 0b1110'0000
                        && (byte2 & 0b1110'0000) == 0b1000'0000) // overlong?
                    || (byte1 == 0b1110'1101
                        && (byte2 & 0b1110'0000) == 0b1010'0000)) { // surrogate?
                    
                    return utf8_str_check::ILL_FORMED;
                }

                if (byte1 == 0b1110'1111
                    && byte2 == 0b1011'1111
                    && (byte3 & 0b1111'1110) == 0b1011'1110) {
                    // U+FFFE or U+FFFF?
                    result = utf8_str_check::WELL_FORMED_NO_CHARACTOR;
                }
                it += 3;
            } else if ((byte1 & 0b1111'1000) == 0b1111'0000) {
                // 1111'0xxx 10xx'xxxx 10xx'xxxx 10xx'xxxx
                if ((it + 3) >= end) {
                    return utf8_str_check::ILL_FORMED;
                }
                uint8_t byte2 = static_cast<uint8_t>(*(it + 1));
                uint8_t byte3 = static_cast<uint8_t>(*(it + 2));
                uint8_t byte4 = static_cast<uint8_t>(*(it + 3));
                if ((byte2 & 0b1100'0000) != 0b1000'0000
                    || (byte3 & 0b1100'0000) != 0b1000'0000
                    || (byte4 & 0b1100'0000) != 0b1000'0000
                    || (byte1 == 0b1111'0000
                        && (byte2 & 0b1111'0000) == 0b1000'0000) // overlong?
                    || (byte1 == 0b1111'0100
                        && byte2 > 0b1000'1111)
                    || byte1 > 0b1111'0100) { // > U+10FFFF?
                    
                    return utf8_str_check::ILL_FORMED;
                }
                
                if ((byte2 & 0b1100'1111) == 0b1000'1111
                    && byte3 == 0b1011'1111
                    && (byte4 & 0b1111'1110) == 0b1011'1110) {
                    // U+nFFFE or U+nFFFF?
                    result = utf8_str_check::WELL_FORMED_NO_CHARACTOR;
                }
                it += 4;
            } else {
                return utf8_str_check::ILL_FORMED;
            }
        }
        return result;
    }

    static constexpr bool is_valid_length(std::string_view str) {
        return str.size() <= 0xFFFF;
    }
};

} // namespace lmqtt