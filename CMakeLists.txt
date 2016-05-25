cmake_minimum_required(VERSION 3.3)
project(sik_radio)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

find_package(Boost REQUIRED COMPONENTS regex)
include_directories( ${Boost_INCLUDE_DIRS} )

set(SOURCE_FILES main.cc shoutcast.cc asio/asio.cc asio/epoll.cc asio/tcp.cc asio/driver.cc)
add_executable(sik_radio ${SOURCE_FILES})
target_link_libraries( sik_radio ${Boost_LIBRARIES} )