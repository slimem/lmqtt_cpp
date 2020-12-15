#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"
#include "lmqtt_properties.h"

namespace lmqtt {

class will_config {
    friend class lmqtt_client_config;
public:
	will_config() = default;
	~will_config() {
		std::cout << "destroyed will config " << this << std::endl;
	}

	[[nodiscard]] reason_code configure_propriety(std::unique_ptr<property::property_data_proxy>&& property) {

		switch (property->get_property_type()) {
		}
	}

private:

	uint32_t _willDelayInterval = 0;
	uint8_t _payloadFormatIndicator = 0;
	uint32_t _messageExpiryInterval = 0;
};

} // namespace lmqtt