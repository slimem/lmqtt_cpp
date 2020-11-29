# lmqtt_cpp_wip (Work In Progress)
A header-only mqtt server written in **c++17** and based on **non-boost asio**.

My main objective is to write a **lightweight**, **scalable** and **high performance** mqtt server with no other libraries than **stl** and **asio**.
This server will be used in the iot-based projects that I'm working on.

I chose to use **non-boost asio** because it will be supported in c++ standard.

## Progress
The server now parses CONNECT packets. In the near future, the server will be able to accept a connection and send pack an ACK packet.
The server will only accept packets with a payload size no more than 1 MB.
