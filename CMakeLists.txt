cmake_minimum_required(VERSION 3.3)
project(sik_radio)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

find_package(Boost REQUIRED COMPONENTS regex)
include_directories( ${Boost_INCLUDE_DIRS} )

set(ASIO_FILES
        asio/asio.cc
        asio/driver.cc
        asio/epoll.cc

        asio/tcp.cc
        asio/udp.cc
)

set(PLAYER_FILES
        player.cc
        shoutcast.cc
)

set(MASTER_FILES
        master.cc
)

add_library(asio STATIC ${ASIO_FILES})

add_executable(player ${PLAYER_FILES})
target_link_libraries(player asio ${Boost_LIBRARIES} )

add_executable(master ${MASTER_FILES})
target_link_libraries(master asio ${Boost_LIBRARIES} )