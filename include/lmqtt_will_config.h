#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"
#include "lmqtt_properties.h"

namespace lmqtt {

class will_config {
    friend class client_config;
public:
	will_config() = default;
	~will_config() {
		std::cout << "destroyed will config " << this << std::endl;
	}

private:

	uint32_t _willDelayInterval = 0; // message must not sent if a session is resumed
	uint8_t _payloadFormatIndicator = 0;
	uint32_t _messageExpiryInterval = 0;
	std::string _contentType;
};

} // namespace lmqtt