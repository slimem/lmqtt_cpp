#pragma once

#include "lmqtt_common.h"
#include "lmqtt_tsqueue.h"
#include "lmqtt_connection.h"
#include "lmqtt_timer.h"

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
		_port(port) {
		// will not use the timer for now
		/*_timer = std::make_shared<lmqtt_timer>(5000, [this] {
				std::cout << "Calling function" << std::endl;
				_timer->reset(_timer->get_time()/2);
				_timer->count--;
				if (_timer->count == 2) {
					std::cout << "Trying to pause thread" << std::endl;
					_timer->stop();
					std::this_thread::sleep_for(2s);
					std::cout << "Trying to resume thread" << std::endl;
					_timer->reset(5000);
					_timer->count = 5;
					_timer->resume();
				}
			}
		);*/
	}

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
							std::move(socket),
							_activeSessions,
							_deletionQueue
						);

					if (on_client_connection(newConnection)) {

						// if the connection was accepted, we push it to our connections container.
						// if the connection was not accepted, the new connection will be destroyed
						// since we are using a smart pointer

						//newConnection->connect_to_client();

						_activeSessions.push_back(std::move(newConnection));

						_activeSessions.back()->connect_to_client(_idCounter++);

						std::cout << "[" << _activeSessions.back()->get_remote_endpoint() << "] Connection Accepted, waiting for identification..\n";

					} else {
						std::cout << "[SERVER] Connection to " << newConnection->get_remote_endpoint() << " Denied. Reason: Reached maximum number of allowed connections\n";
						_deletionQueue.push_back(newConnection);
					}

				} else {
					// error occurred during acceptance
					std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
				}

				wait_for_clients();
			}
		);
	}
	
public:
	// can be used to change how many messages can be processed at a time
	void update(size_t maxMessages = -1) {

		_deletionQueue.wait();
		//_messages.wait();

		if (!_deletionQueue.empty()) {
			std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();
			//std::cout << "Detected deletion queue not empty\n";
			auto connection = _deletionQueue.pop_front();
			connection->shutdown();
			_activeSessions.find_and_erase(connection);
			//std::chrono::system_clock::time_point timeEnd = std::chrono::system_clock::now();
			//std::cout << "Deletion from queue took " << std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count() << " us\n";
		}
	}

protected:

	bool on_client_connection(std::shared_ptr<connection> connection) {
		if (_activeSessions.size() > 5) {
			return false;
		}
		return true;
	}

	void client_timeout_handler(std::error_code ec) {
		if (!ec) {
			std::cout << "This timer for client has expired\n";
		}
	}

protected:

	// container for active connections
	ts_queue<std::shared_ptr<connection>> _activeSessions;

	// container for connections scheduled for deletion
	ts_queue<std::shared_ptr<connection>> _deletionQueue;

	// container for messages to be treated
	// for now, it is a queue of strings
	ts_queue<std::string_view> _messages;

	// timeout
	std::shared_ptr<lmqtt_timer> _timer;

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
