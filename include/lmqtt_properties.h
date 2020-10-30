#pragma once

#include "lmqtt_common.h"

namespace lmqtt {

namespace property {

class property_data_proxy {
    ~property_data_proxy() = default;
};

template <typename T>
class property_data : public property_data_proxy {
    T _data;
    T& get_data() const noexcept {
        return T;
    }
    void set_data(T& data) noexcept {
        _data = data;
    }
};
} //namespace property

} // namespace lmqtt
