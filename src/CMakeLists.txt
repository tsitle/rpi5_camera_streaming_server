cmake_minimum_required(VERSION 3.10)

project( HttpCamServer )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# '-fcompare-debug-second' turns off annoying compiler 'Notes'
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fcompare-debug-second")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package( OpenCV REQUIRED )
find_package( Threads REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( HttpCamServer
    main.cpp shared.cpp cfgfile.cpp
    http/http_tcp_server.cpp http/http_clienthandler.cpp
    frame/frame_producer.cpp frame/frame_consumer.cpp frame/frame_processor.cpp frame/frame_queue.cpp
  )

target_link_libraries( HttpCamServer PRIVATE ${OpenCV_LIBS} Threads::Threads )
