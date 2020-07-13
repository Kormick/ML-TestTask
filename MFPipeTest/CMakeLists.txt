project(MFPipe_Test)

find_package(Threads)

set(SOURCES
	MFPipeImpl.cpp
	pipe/PipeParser.cpp
	pipe/PipeReader.cpp
	pipe/PipeWriter.cpp
	pipe/UnixIoPipe.cpp
	pipe/WinIoPipe.cpp
	udp/UnixIoUdp.cpp
	udp/WinIoUdp.cpp
	unittest_mfpipe.cpp
	)

set(HEADERS
	MFPipe.h
	MFPipeImpl.h
	MFTypes.h
	IoInterface.hpp
	pipe/PipeParser.hpp
	pipe/PipeReader.hpp
	pipe/PipeWriter.hpp
	pipe/UnixIoPipe.hpp
	pipe/WinIoPipe.hpp
	tests/Parser.hpp
	tests/Pipe.hpp
	tests/Udp.hpp
	udp/UnixIoUdp.hpp
	udp/WinIoUdp.hpp
	)

include_directories(
	.
	pipe
	tests
	udp
	)

if(WIN32)
	list(REMOVE_ITEM HEADERS pipe/UnixIoPipe.hpp)
	list(REMOVE_ITEM SOURCES pipe/UnixIoPipe.cpp)

	list(REMOVE_ITEM HEADERS udp/UnixIoUdp.hpp)
	list(REMOVE_ITEM SOURCES udp/UnixIoUdp.cpp)
else()
	list(REMOVE_ITEM HEADERS pipe/WinIoPipe.hpp)
	list(REMOVE_ITEM SOURCES pipe/WinIoPipe.cpp)

	list(REMOVE_ITEM HEADERS udp/WinIoUdp.hpp)
	list(REMOVE_ITEM SOURCES udp/WinIoUdp.cpp)
endif()

add_executable(MFPipe_Test ${SOURCES} ${HEADERS})

target_link_libraries(MFPipe_Test ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
	target_link_libraries(MFPipe_Test ws2_32)
endif()
