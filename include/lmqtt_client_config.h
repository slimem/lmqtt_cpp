#pragma once

#include "lmqtt_common.h"
#include "lmqtt_types.h"
#include "lmqtt_properties.h"
#include "lmqtt_will_config.h"

namespace lmqtt {

class client_config : public std::enable_shared_from_this<client_config> {
	friend class lmqtt_packet;
	friend class connection;
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
		case payload::payload_type::WILL_TOPIC:
		{
			if (_willFlag != 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			payload::payload<std::string_view>* realData =
				static_cast<payload::payload<std::string_view>*>(payload.get());
			if (realData->check_data_type(payload::payload_type::WILL_TOPIC) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_topic = realData->get_data();
			break;
		}
		case payload::payload_type::WILL_PAYLOAD:
		{
			if (_willFlag != 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			payload::payload<std::vector<uint8_t>>* realData =
				static_cast<payload::payload<std::vector<uint8_t>>*>(payload.get());
			if (realData->check_data_type(payload::payload_type::WILL_PAYLOAD) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_willPayload = std::move(realData->get_data());
			break;
		}
		case payload::payload_type::USER_NAME:
		{
			if (_userNameFlag != 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			payload::payload<std::string_view>* realData =
				static_cast<payload::payload<std::string_view>*>(payload.get());
			if (realData->check_data_type(payload::payload_type::USER_NAME) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_userName = realData->get_data();
			break;
		}
		case payload::payload_type::PASSWORD:
		{
			if (_willFlag != 1) {
				return reason_code::PROTOCOL_ERROR;
			}
			payload::payload<std::vector<uint8_t>>* realData =
				static_cast<payload::payload<std::vector<uint8_t>>*>(payload.get());
			if (realData->check_data_type(payload::payload_type::PASSWORD) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_password = std::move(realData->get_data());
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
		case property::property_type::RESPONSE_TOPIC:
		{
			property::property_data<std::string_view>* realData =
				static_cast<property::property_data<std::string_view>*>(property.get());
			if (realData->check_data_type(property::property_type::RESPONSE_TOPIC) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_responseTopic = realData->get_data();
			break;
		}
		case property::property_type::CORRELATION_DATA:
		{
			property::property_data<std::vector<uint8_t>>* realData =
				static_cast<property::property_data<std::vector<uint8_t>>*>(property.get());
			if (realData->check_data_type(property::property_type::CORRELATION_DATA) != return_code::OK) {
				return reason_code::PROTOCOL_ERROR;
			}
			_willCfg->_correlationData = std::move(realData->get_data());
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
			_willCfg->_userProprieties.emplace_back(std::make_pair(properties.first, properties.second));
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

	[[nodiscard]] uint32_t get_property_size(property::property_type ptype) {
		using namespace property;
		if (types_utils::is_property_fixed(ptype)) {
			return (1 + static_cast<uint8_t>(types_utils::get_property_data_type(ptype)));
		} else {
			switch (ptype) {
			case property_type::ASSIGNED_CLIENT_ID: return (1 + 2 + _clientId.size());
			case property_type::REASON_STRING:		return (1 + 2 + _reasonString.size());
			case property_type::USER_PROPERTY:
			{
				//TODO: To be removed in the future (unless we need to really sed user properties)
				uint32_t totalSize = 1;
				for (auto& p : _userProprieties) {
					totalSize += (p.first.size() + 2);
					totalSize += (p.second.size() + 2);
				}
				if (!totalSize) {
					return 0;
				} else {
					return totalSize;
				}
			}
			case property_type::RESPONSE_INFORMATION: return 0; // TODO: Not yet supported
			case property_type::SERVER_REFERENCE: return 0; // TODO: Not yet supported
			case property_type::AUTHENTICATION_METHOD: return 0; // TODO: Not yet supported
			case property_type::AUTHENTICATION_DATA: return 0; // TODO: Not yet supported
			default:
				std::cerr << "[WARNING] -- Precomputing size for unknown property " << std::hex << static_cast<int>(ptype) << "\n";
				return 0;
			}
		}
	}

	[[nodiscard]] return_code fill_property(uint8_t* buff, uint32_t buffSize, property::property_type ptype, uint32_t propertySize) {

		using namespace property;
		propertySize = get_property_size(ptype);

		switch (ptype) {
		case property_type::SESSION_EXPIRY_INTERVAL:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			
			if (write_property_to_buffer<uint32_t>(buff + 1, buffSize - 1, _sessionExpiryInterval) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::RECEIVE_MAXIMUM:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint16_t>(buff + 1, buffSize - 1, _receiveMaximum) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::MAXIMUM_QOS:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, _maximumQos) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::RETAIN_AVAILABLE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, _retainAvailable) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::MAXIMUM_PACKET_SIZE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint32_t>(buff + 1, buffSize - 1, _maximumPacketSize) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::ASSIGNED_CLIENT_ID:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<std::string&>(buff + 1, buffSize - 1, _clientId) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::TOPIC_ALIAS_MAXIMUM:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint16_t>(buff + 1, buffSize - 1, _topicAliasMaximum) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::REASON_STRING:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<std::string&>(buff + 1, buffSize - 1, _reasonString) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::USER_PROPERTY:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			for (auto& prop : _userProprieties) {
				if (write_property_to_buffer<std::pair<const std::string, const std::string>&>(buff + 1, buffSize - 1, prop) != return_code::OK) {
					return return_code::FAIL;
				}
			}
			break;
		}
		case property_type::WILDCARD_SUBSCRIPTION_AVAILABLE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, _wildcardSubscription) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::SUBSCRIPTION_ID_AVAILABLE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, 1) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::SHARED_SUBSCRIPTION_AVAILABLE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, 1) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::SERVER_KEEP_ALIVE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint16_t>(buff + 1, buffSize - 1, _keepAlive) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::RESPONSE_INFORMATION:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, _requestResponseInformation) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		case property_type::SERVER_REFERENCE:
		{
			// check if the buffer can hold this property
			if (buffSize < propertySize) {
				return return_code::FAIL;
			}

			buff[0] = static_cast<uint8_t>(ptype);
			if (write_property_to_buffer<uint8_t>(buff + 1, buffSize - 1, _requestResponseInformation) != return_code::OK) {
				return return_code::FAIL;
			}
			break;
		}
		}

		return return_code::OK;
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
	std::string _userName;
	std::vector<uint8_t> _password;

	uint8_t _maximumQos = 1;
	uint8_t _retainAvailable = 1;
	uint8_t _wildcardSubscription = 1;

    std::string _clientId;

	std::string _reasonString{ "Unspecified Error" };
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