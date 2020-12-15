#pragma once

#include "lmqtt_common.h"
#include "lmqtt_packet.h"
#include "lmqtt_reason_codes.h"
#include "lmqtt_timer.h"
#include "lmqtt_client_config.h"

namespace lmqtt {

class connection : public std::enable_shared_from_this<connection> {
	 // enable_shared_from_this will allow us to create a shared pointer "this" internally
	 // in other word, a shared this pointer other than a raw pointer
public:

	connection(
		asio::io_context& context,
		asio::ip::tcp::socket socket,
		ts_queue<std::shared_ptr<connection>>& activeConnections, // all active connections
		ts_queue<std::shared_ptr<connection>>& deletionQueue // connections scheduled for deletion
	) :
		_context(context),
		_socket(std::move(socket)),
		_activeConnections(activeConnections),
		_deletionQueue(deletionQueue),
		_clientCfg(std::make_shared<client_config>()),
		_connTimer(nullptr)
	{
		_inPacket._clientCfg = _clientCfg;
		_outPacket._clientCfg = _clientCfg;
	}

	virtual ~connection() {
		//std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();
		std::cout << "[-------] Destroyed client object " << this << std::endl;
		//std::chrono::system_clock::time_point timeEnd = std::chrono::system_clock::now();
		//std::cout << "Deletion Took " << std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count() << " us\n";
	}

public:
	[[nodiscard]] uint32_t get_id() const noexcept {
		return _id;
	}

	void connect_to_client(size_t timeout) noexcept {
		if (_socket.is_open()) {
			//_id = id;
			_connTimer = std::unique_ptr<lmqtt_timer>(new lmqtt_timer(
				timeout,
				[this]() {
					if (!_receivedData) {
						shutdown();
						schedule_for_deletion();
					}
				}
			));
				
			// read availabe messages
			read_fixed_header();
		}
	}

	void disconnect() {
		if (is_connected()) {
			//the context can close the socket whenever it is available
			asio::post(
				_context,
				[this]() {
					_socket.close();
				}
			);
		}
	}

	// close connection immediately without waiting for context.
	// the socket should never outlive its context, so closing a socket before
	// deleting the context is mendatory
	void shutdown() {
		if (is_connected()) {
			std::error_code ec;
			_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
			if (ec) {
				std::cout << "Could not delete client, reason: " << ec.message() << std::endl;
			}
		}
	}

	bool is_connected() const noexcept {
		return _socket.is_open();
	};

private:
	// async method: prime the context ready to read a packet header
	void read_fixed_header() {
		asio::async_read(
			_socket,
			asio::buffer(
				&_inPacket._header._controlField,
				sizeof(uint8_t)
			),
			[this](std::error_code ec, size_t length) {
				if (!ec) {

					_receivedData = true;

					// we identify the packet type
					const reason_code rcode = _inPacket.create_fixed_header();

					if (rcode == reason_code::MALFORMED_PACKET
						|| rcode == reason_code::PROTOCOL_ERROR) {
						_socket.close();
					}

					// on first connection, only accept CONNECT packets
					if (_isFirstPacket) {
						if (_inPacket._type != packet_type::CONNECT) {
							_socket.close();
							return;
						}
						_isFirstPacket = false;
					}

					// read packet length
					uint8_t mul = 1;
					uint32_t packetLenth = 0;

					// the following section is used to decode the packet length, where we use the asio buffer to read
					// one byte at a time. So if the MSB bit = 1, we read the next byte and so and fourth. If we read 
					// more than 4 (mul is multiplied 4 times) it means that the packet is malformed.
					reason_code rCode = reason_code::SUCCESS;
					for (uint32_t offset = 0; offset < 4; ++offset) {
						uint8_t nextByte = read_byte();
						_inPacket._header._packetLen += (nextByte & 0x7f) * mul;
						if (mul > 0x200000) { // 128 * 128 * 128
							rCode = reason_code::MALFORMED_PACKET;
						}
						mul *= 0x80; // prepare for next byte
						if (!(nextByte & 0x80) || (rCode == reason_code::MALFORMED_PACKET)) break;
					}

					if (rCode == reason_code::MALFORMED_PACKET) {
						_socket.close();
						return;
					}

					// only allow packets with a certain size
					if (_inPacket._header._packetLen > PACKET_SIZE_LIMIT) {
						std::cout << "[" << _id << "] Closed connection. Reason: Packet size limit exceeded\n";
						_socket.close();
						return;
					}

					// resize packet body to hold the rest of the data
					_inPacket._body.resize(_inPacket._header._packetLen);
					
					read_packet_body();

				} else {
					std::cout << "[" << _id << "] Reading Header Failed: " << ec.message() << "\n";
					_socket.close();
				}
			}
		);
	}

	[[nodiscard]] uint8_t read_byte() {
		uint8_t byte;
		asio::async_read(
			_socket,
			asio::buffer(
				&byte,
				sizeof(uint8_t)
			),
			[this](std::error_code ec, size_t length) {
				if (ec) {
					std::cout << "[" << _id << "] Reading Byte Failed: " << ec.message() << "\n";
					_socket.close();
				}
			}
		);
		return byte;
	}

	void read_packet_body() {
		asio::async_read(
			_socket,
			asio::buffer(
				_inPacket._body.data(),
				_inPacket._body.size()
			),
			[this](std::error_code ec, size_t length) {
				if (!ec) {

					reason_code rcode;
					switch (_inPacket._type) {
					case packet_type::CONNECT:
					{
						rcode = _inPacket.decode_connect_packet_body();
						if (rcode != reason_code::SUCCESS) {
							_socket.close();
							schedule_for_deletion();
							return;
						}
						configure_client();
						break;
					}
					}

					//read_packet();

					/*payload::payload_proxy* data = _inPacket._payloads[0].get();
					payload::payload<std::string_view>* realData =
						static_cast<payload::payload<std::string_view>*>(data);

					std::cout << "[" << realData->get_data() << "] Connection Approved\n";*/

					_socket.close();
					schedule_for_deletion();
					return;


					/*if (_inPacket.decode_packet_body() != reason_code::SUCCESS) {
						// According to packet type, we create our ACK packet to be sent to the client

						_socket.close();
					}*/
					
					//read_fixed_header();

				} else {
					std::cout << "[" << _id << "] Reading pakcet body Failed: " << ec.message() << "\n";
					_socket.close();
				}
			}
		);
	}

	void read_packet() {

		switch (_inPacket._type) {
		case packet_type::CONNECT:
		{
			configure_client();

			break;
		}


		}

		read_fixed_header();
	}

	void configure_client() {
	/*	for (uint8_t i = 0; i < _inPacket._payloads.size(); ++i) {

			auto ptype = _inPacket._payloads[i]->get_payload_type();



		}*/
	}

	void schedule_for_deletion() {
		_deletionQueue.push_back(shared_from_this());
	}


public:

	std::string get_remote_endpoint() const {
		return _socket.remote_endpoint().address().to_string();
	}

	asio::ip::tcp::socket& socket() {
		return _socket;
	}

protected:
	// each connection has a unique socket
	asio::ip::tcp::socket _socket;

	// context
	asio::io_context& _context;
	
	ts_queue<std::shared_ptr<connection>>& _activeConnections;
	ts_queue<std::shared_ptr<connection>>& _deletionQueue;
	
	// connection ID
	uint32_t _id = 0;

	// on connect, we expect a connect packet
	bool _isFirstPacket = true;
	std::atomic<bool> _receivedData{false};

	lmqtt_packet _inPacket;
	lmqtt_packet _outPacket;

	std::unique_ptr<lmqtt_timer> _connTimer;

	std::shared_ptr<client_config> _clientCfg;

};

} // namespace lmqtt
