cmake_minimum_required(VERSION 3.3)
project(sik_radio)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

set(SOURCE_FILES main.cc asio/asio.cc asio/epoll.cc asio/tcp.cc asio/driver.cc)
add_executable(sik_radio ${SOURCE_FILES})