#pragma once

#include "lmqtt_common.h"
#include "lmqtt_tsqueue.h"
#include "lmqtt_connection.h"

namespace lmqtt {

class lmqtt_server {
public:
	lmqtt_server(
		uint16_t port
	) : 
		_acceptor(
			_context,
			asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)
		),
		_port(port) {}

	virtual ~lmqtt_server() {
		stop();
	}

	[[nodiscard]] bool start() {
		try {

			wait_for_clients();

			_thContext = std::thread([this]() {_context.run(); });

		} catch (std::exception& e) {
			std::cerr << "[SERVER] Could Not Start Server. Reason:\n" << e.what() << "\n";
			return false;
		}

		std::cout << "[SERVER] Successfully Started LMQTT Server\n";
		std::cout << "[SERVER] Listening on port " << _port << "\n";
		return true;
	}

	void stop() {
		// attempt to stop the asio context, then join its thread
		_context.stop();
		// maybe the context will be busy, so we have to wait
		// for it to finish using std::thread.join()
		if (_thContext.joinable()) {
			_thContext.join();
		}

		std::cout << "[SERVER] Successfully Stopped LMQTT Server.\n";
	}

protected:
	// async method: wait for connection
	// It's here where all the magic happens
	void wait_for_clients() {
		_acceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket) {
				if (!ec) {
					// if the connection attempt is successful
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

					// create a new connection object for this specific client
					// since we used a smart pointer, the new connection will be destroyed
					// when it falls out of scope, in case the connection was not accepted
					std::shared_ptr<connection> newConnection =
						std::make_shared<connection>(
							_context,
							std::move(socket)
						);

					if (on_client_connection(newConnection)) {

						// if the connection was accepted, we push it to our connections container.
						// if the connection was not accepted, the new connection will be destroyed
						// since we are using a smart pointer
						_activeConnections.push_back(std::move(newConnection));

						_activeConnections.back()->connect_to_client(_idCounter++);

						std::cout << "[" << _activeConnections.back()->get_id() << "] Connection Approved\n";

					} else {
						std::cout << "[SERVER] Connection to " << socket.remote_endpoint() << " Denied. Reason: Reached maximum number of allowed connections\n";
					}

				} else {
					// error occurred during acceptance
					std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
				}

				wait_for_clients();
			}
		);
	}
	
	// can be used to change how many messages can be processed at a time
	void update(size_t maxMessages = -1) {
	}

protected:

	bool on_client_connection(std::shared_ptr<connection> connection) {
		if (_activeConnections.size() > 2) {
			return false;
		}
		return true;
	}

protected:

	// container for active connections
	std::deque<std::shared_ptr<connection>> _activeConnections;

	// for the server to actually run with asio
	asio::io_context _context;
	std::thread _thContext;

	// since we dont need sockets, we need acceptors
	asio::ip::tcp::acceptor _acceptor;

	// Server will identify client by this ID, and use them for reporting.
	// also, it will help with some data charting later
	uint32_t _idCounter = 0;
	uint16_t _port = 0;
};

} // namespace lmqtt
