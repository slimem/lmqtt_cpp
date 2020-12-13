#pragma once

#include "lmqtt_common.h"

namespace lmqtt {

class client_config : public std::enable_shared_from_this<client_config> {
	friend class lmqtt_packet;
public:
	client_config() = default;
	~client_config() {
		std::cout << "destroyed client config " << this << std::endl;
	}

public:
	void set_client_id(const std::string& clientId) {
		_clientId = clientId;
	}

private:
    std::string _clientId;
	uint32_t _sessionExpiryInterval = 0;
	uint32_t _maximumPacketSize = 0;
	uint8_t _qos = 0;
	uint8_t _cleanStart = 0;
	uint8_t _willFlag = 0;
	uint8_t _willQos = 0;
	uint8_t _willRetain = 0;
	uint8_t _passwordFlag = 0;
	uint8_t _userNameFlag = 0;
	uint16_t _keepAlive = 0; // zero means infinite

};

} // namespace lmqtt