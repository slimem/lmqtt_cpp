# lmqtt_cpp_wip (Work In Progress)
A header-only mqtt server written in **c++17** and based on **non-boost asio**.

My main objective is to write a **lightweight**, **scalable** and **high performance** mqtt server with no other libraries than **stl** and **asio**.
This server will be used in the iot-based projects that I'm working on.

I chose to use **non-boost asio** because it will be supported in c++ standard.

## Progress
The server now parses CONNECT packets. In the near future, the server will be able to accept a connection and send pack an ACK packet.
The server will only accept packets with a payload size no more than 1 MB.

I am currently able to establish a CONNECT CONNACK packet transaction but the client disconnects after receiving the CONNACK packet, so I am currently debugging that. Also, I am working on implementing a better error reporting system based on std::error_code with holds both the enum to error type and the error message itself. This will become handy in the future since I will send the reason informations with the ACK packet.

The lmqtt packet is implemented as follows (in lmqtt_packet.h)
```cpp
class lmqtt_packet {
    fixed_header _header {};
    std::vector<uint8_t> _body;
```
At connection, the packet parsing is done in two steps: First, the fixed header is read and parsed, then the body.
In lmqtt_connection.h:
```cpp
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
```

The next step is to impelement a thread-safe priority queue that handles timeouts (keep alive/ session expiration) for sessions.

To compile and run the server, non-boost asio (header only) must be linked.
The server is compatible with any mqtt v5 client.
To explore the example and how a CONNECT packet is parsed, run it in debug mode, put a break point in the follwing section and run line by line (in lmqtt_packet.h)
```cpp
    [[nodiscard]] const reason_code decode_connect_packet_body() {
        std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();
        // TODO: use a uint8_t* and advance it until we reach uint8_t* + body().size()
        // This way, we can avoid indexed access alltogether.
        // For now, we use an indexed access since we are only decoding CONNECT packet
        // for a proof of concept. Then in the future, to support all packet types, we
        // must decode them in a more generic manner.

        uint8_t* ptr = _body.data();

        // byte 0 : length MSB
        // byte 1 : length LSB
        const uint8_t protocolNameLen = *(ptr + 1) + *ptr;
```
