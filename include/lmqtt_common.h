#pragma once
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <deque>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <array>

//for debugging
#include <bitset>

#ifdef _WIN32
#define _WIN32_WINTT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
