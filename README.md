# lmqtt_cpp_wip (Work In Progress)
A header-only mqtt server written in **c++17** and based on **non-boost asio**.

My main objective is to write a **lightweight**, **scalable** and **high performance** mqtt server with no other libraries than **stl** and **asio**.
This server will be used in the iot-based projects that I'm working on.

I chose to use **non-boost asio** because it will be supported in c++ standard.

## Progress
The server now parses CONNECT packets. In the near future, the server will be able to accept a connection and send pack an ACK packet.
The server will only accept packets with a payload size no more than 1 MB.

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
