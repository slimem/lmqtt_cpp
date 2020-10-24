#pragma once

#include "lmqtt_common.h"
#include "lmqtt_packet.h"
#include "lmqtt_reason_codes.h"

namespace lmqtt {

	// this connection class will behave differently, if it's owned by a client
	// or if it's owned by a server
	class connection : public std::enable_shared_from_this<connection> {
		 // enable_shared_from_this will allow us to create a shared pointer "this" internally
		 // in other word, a shared this pointer other than a raw pointer
	public:
		enum class owner {
			server,
			client
		};

		connection(
			owner parent,
			asio::io_context& context,
			asio::ip::tcp::socket socket
		) :
			_context(context),
			_socket(std::move(socket))
		{
			_owner = parent;
			_byte = 0U;
		}

		virtual ~connection() {}

	public:
		[[nodiscard]] uint32_t getID() const noexcept {
			return _id;
		}

		void connectToClient(uint32_t id = 0) noexcept {
			if (_owner == owner::server) {
				if (_socket.is_open()) {
					_id = id;

					// read availabe messages
					read_fixed_header();
				}
			}
		}
	
		void connectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
			// only relevent to clients
			if (_owner == owner::client) {

				// try to async connect to an endpoint with the socket in the connection
				// object
				asio::async_connect(
					_socket,
					endpoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
						if (!ec) {
							read_fixed_header();
						}
					}
				);
			}
		}

		void disconnect() {
			if (isConnected()) {
				//the context can close the socket whenever it is available
				asio::post(
					_context,
					[this]() {
						_socket.close();
					}
				);
			}
		}

		bool isConnected() const noexcept {
			return _socket.is_open();
		};
	
	private:
		// async method: prime the context ready to read a message header
		void read_fixed_header() {
			asio::async_read(
				_socket,
				asio::buffer(
					&_byte,
					sizeof(uint8_t)
				),
				[this](std::error_code ec, size_t length) {
					if (!ec) {

						// we identify the packet type
						const reason_code rcode = _tempPacket.create_fixed_header(_byte);

						if (rcode == reason_code::MALFORMED_PACKET
							|| rcode == reason_code::PROTOCOL_ERROR) {
							_socket.close();
						}

						// read packet length
						read_packet_length();
						std::cout << _tempPacket;
						_socket.close();

					} else {
						std::cout << "[" << _id << "] Reading Header Failed: " << ec.message() << "\n";
						_socket.close();
					}
				}
			);
		}

		void read_packet_length() {
			asio::async_read(
				_socket,
				asio::buffer(
					&_byte,
					sizeof(uint8_t)
				),
				[this](std::error_code ec, size_t length) {
					if (!ec) {
						bool next = false;
						if (_tempPacket.decode_packet_length(_byte, next) == reason_code::MALFORMED_PACKET) {
							_socket.close();
						}
						if (next) {
							read_packet_length();
						}
						std::cout << "Finished, disconnecting\n";
						std::cout << _tempPacket._header._packetLen << std::endl;
						_socket.close();

					}
				}
			);
		}

	protected:
		// each connection has a unique socket
		asio::ip::tcp::socket _socket;
	
		// context
		asio::io_context& _context;
	
		// owned by server by default
		owner _owner = owner::server;
		uint32_t _id = 0;

		uint8_t _byte;
		packet _tempPacket;
	};

} // namespace lmqtt
