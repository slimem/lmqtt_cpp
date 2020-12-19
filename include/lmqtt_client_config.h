#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"
#include "lmqtt_properties.h"
#include "lmqtt_will_config.h"

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

	[[nodiscard]] reason_code configure_propriety(std::unique_ptr<property::property_data_proxy>&& property) {

		switch (property->get_property_type()) {
		case property::property_type::SESSION_EXPIRY_INTERVAL:
		{
			property::property_data<uint32_t>* realData =
				static_cast<property::property_data<uint32_t>*>(property.get());
			if (realData->check_data_type(property::property_type::SESSION_EXPIRY_INTERVAL) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_sessionExpiryInterval = realData->get_data();
			break;
		}
		case property::property_type::RECEIVE_MAXIMUM:
		{
			property::property_data<uint16_t>* realData =
				static_cast<property::property_data<uint16_t>*>(property.get());
			if (realData->check_data_type(property::property_type::RECEIVE_MAXIMUM) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_receiveMaximum = realData->get_data();
			if (_receiveMaximum == 0) {
				return reason_code::PROTOCOL_ERROR;
			}
			break;
		}
		case property::property_type::MAXIMUM_PACKET_SIZE:
		{
			property::property_data<uint32_t>* realData =
				static_cast<property::property_data<uint32_t>*>(property.get());
			if (realData->check_data_type(property::property_type::MAXIMUM_PACKET_SIZE) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_maximumPacketSize = realData->get_data();
			if (_maximumPacketSize == 0) {
				return reason_code::PROTOCOL_ERROR;
			}
			break;
		}
		case property::property_type::TOPIC_ALIAS_MAXIMUM:
		{
			property::property_data<uint16_t>* realData =
				static_cast<property::property_data<uint16_t>*>(property.get());
			if (realData->check_data_type(property::property_type::TOPIC_ALIAS_MAXIMUM) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_topicAliasMaximum = realData->get_data();
			break;
		}
		case property::property_type::REQUEST_RESPONSE_INFORMATION:
		{
			property::property_data<uint8_t>* realData =
				static_cast<property::property_data<uint8_t>*>(property.get());
			if (realData->check_data_type(property::property_type::REQUEST_RESPONSE_INFORMATION) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_requestResponseInformation = realData->get_data();
			if (_requestResponseInformation > 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			break;
		}
		case property::property_type::REQUEST_PROBLEM_INFORMATION:
		{
			property::property_data<uint8_t>* realData =
				static_cast<property::property_data<uint8_t>*>(property.get());
			if (realData->check_data_type(property::property_type::REQUEST_PROBLEM_INFORMATION) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_requestProblemInformation = realData->get_data();
			if (_requestProblemInformation > 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			break;
		}
		case property::property_type::USER_PROPERTY:
		{
			property::property_data<std::pair<std::string_view, std::string_view>>* realData =
				static_cast<property::property_data<std::pair<std::string_view, std::string_view>>*>(property.get());
			if (realData->check_data_type(property::property_type::USER_PROPERTY) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			auto properties = realData->get_data();
			_userProprieties.emplace_back(std::make_pair(properties.first, properties.second));
			break;
		}
		case property::property_type::AUTHENTICATION_METHOD:
		{
			property::property_data<std::string_view>* realData =
				static_cast<property::property_data<std::string_view>*>(property.get());
			if (realData->check_data_type(property::property_type::AUTHENTICATION_METHOD) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_authMethod = realData->get_data();
			break;
		}
		case property::property_type::AUTHENTICATION_DATA:
		{
			property::property_data<std::vector<uint8_t>>* realData =
				static_cast<property::property_data<std::vector<uint8_t>>*>(property.get());
			if (realData->check_data_type(property::property_type::AUTHENTICATION_DATA) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			if (_authMethod.empty()) {
				return reason_code::PROTOCOL_ERROR;
			}
			_authData = std::move(realData->get_data());
			break;
		}
		default:
		{
			// never reached
			return reason_code::PROTOCOL_ERROR;
		}
		}

		return reason_code::SUCCESS;
	}

	[[nodiscard]] reason_code configure_payload(std::unique_ptr<payload::payload_proxy>&& payload) {

		switch (payload->get_payload_type()) {
		case payload::payload_type::CLIENT_ID:
		{
			payload::payload<std::string_view>* realData =
				static_cast<payload::payload<std::string_view>*>(payload.get());
			if (realData->check_data_type(payload::payload_type::CLIENT_ID) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_clientId = realData->get_data();
			break;
		}
		default:
		{
			// never reached
			return reason_code::PROTOCOL_ERROR;
		}
		}
		return reason_code::SUCCESS;
	}

	void init_will_cfg() noexcept {
		_willCfg = std::unique_ptr<will_config>(new will_config);
	}

	[[nodiscard]] reason_code configure_will_propriety(std::unique_ptr<property::property_data_proxy>&& property) {

		if (!_willCfg) {
			// this shouldnt happen, unless if the will flag is not set but there's a will payload
			return reason_code::MALFORMED_PACKET;
		}

		switch (property->get_property_type()) {
		case property::property_type::WILL_DELAY_INTERVAL:
		{
			property::property_data<uint32_t>* realData =
				static_cast<property::property_data<uint32_t>*>(property.get());
			if (realData->check_data_type(property::property_type::WILL_DELAY_INTERVAL) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_willDelayInterval = realData->get_data();
			break;
		}
		case property::property_type::PAYLOAD_FORMAT_INDICATOR:
		{
			property::property_data<uint8_t>* realData =
				static_cast<property::property_data<uint8_t>*>(property.get());
			if (realData->check_data_type(property::property_type::PAYLOAD_FORMAT_INDICATOR) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_payloadFormatIndicator = realData->get_data();
			if (_willCfg->_payloadFormatIndicator > 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			break;
		}
		case property::property_type::MESSAGE_EXPIRY_INTERVAL:
		{
			property::property_data<uint32_t>* realData =
				static_cast<property::property_data<uint32_t>*>(property.get());
			if (realData->check_data_type(property::property_type::MESSAGE_EXPIRY_INTERVAL) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_messageExpiryInterval = realData->get_data();
			break;
		}
		case property::property_type::CONTENT_TYPE:
		{
			property::property_data<std::string_view>* realData =
				static_cast<property::property_data<std::string_view>*>(property.get());
			if (realData->check_data_type(property::property_type::CONTENT_TYPE) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_contentType = realData->get_data();
			break;
		}
		}
		return reason_code::SUCCESS;
	}

private:

	// client properties in this order
	uint32_t _sessionExpiryInterval = 0;
	uint16_t _receiveMaximum = 0;
	uint32_t _maximumPacketSize = 0;
	uint16_t _topicAliasMaximum = 0;
	uint8_t _requestResponseInformation = 0; // only applicable to CONNACK
	uint8_t _requestProblemInformation = 0; // applicable to other packets if allowed
	std::vector<std::pair<const std::string, const std::string>> _userProprieties;
	std::string _authMethod;
	std::vector<uint8_t> _authData;

    std::string _clientId;

	uint8_t _qos = 0;
	uint8_t _cleanStart = 0;
	uint8_t _willFlag = 0;
	uint8_t _willQos = 0;
	uint8_t _willRetain = 0;
	uint8_t _passwordFlag = 0;
	uint8_t _userNameFlag = 0;
	uint16_t _keepAlive = 0; // zero means infinite

	std::unique_ptr<will_config> _willCfg{ nullptr };
};

} // namespace lmqtt