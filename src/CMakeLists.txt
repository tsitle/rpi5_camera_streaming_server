cmake_minimum_required(VERSION 3.10)

project( HttpCamServer )

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(UNIX AND NOT APPLE)
	# for Linux, BSD, Solaris, Minix

	# '-fcompare-debug-second' turns off annoying compiler 'Notes'
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -fcompare-debug-second")
else()
	# for macOS

	set(CMAKE_CXX_FLAGS "-Wall -Wextra")
	set(OpenCV_DIR "../build_opencv/4.11.0/build")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package( OpenCV REQUIRED )
find_package( Threads REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( HttpCamServer
		main.cpp
		shared.cpp
		cfgfile.cpp
		cputemp/cputemp.cpp
		http/http_tcp_server.cpp
		http/http_clienthandler.cpp
		http/http_handleroute.cpp
		http/http_handleroute_get.cpp
		http/http_handleroute_post.cpp
		frame/frame_producer.cpp
		frame/frame_consumer.cpp
		frame/frame_processor.cpp
		frame/frame_queue_jpeg.cpp
		frame/frame_queue_rawinput.cpp
		frame/subprocessor/subproc.cpp
		frame/subprocessor/subproc_bnc.cpp
		frame/subprocessor/subproc_calibrate.cpp
		frame/subprocessor/subproc_flip.cpp
		frame/subprocessor/subproc_grid.cpp
		frame/subprocessor/subproc_pt.cpp
		frame/subprocessor/subproc_roi.cpp
		frame/subprocessor/subproc_scale.cpp
		frame/subprocessor/subproc_text.cpp
		frame/subprocessor/subproc_tr.cpp
		md5/md5.cpp
	)

target_link_libraries( HttpCamServer PRIVATE ${OpenCV_LIBS} Threads::Threads )
