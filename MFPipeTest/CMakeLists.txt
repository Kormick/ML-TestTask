project(MFPipe_Test)

find_package(Threads)

set(SOURCES
	pipe/PipeParser.cpp
	pipe/PipeReader.cpp
	pipe/PipeWriter.cpp
	pipe/UnixIoPipe.cpp
	pipe/WinIoPipe.cpp
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
	)

include_directories(
	.
	pipe
	tests
	)

if(WIN32)
	list(REMOVE_ITEM HEADERS pipe/UnixIoPipe.hpp)
	list(REMOVE_ITEM SOURCES pipe/UnixIoPipe.cpp)
else()
	list(REMOVE_ITEM HEADERS pipe/WinIoPipe.hpp)
	list(REMOVE_ITEM SOURCES pipe/WinIoPipe.cpp)
endif()

add_executable(MFPipe_Test ${SOURCES} ${HEADERS})

target_link_libraries(MFPipe_Test ${CMAKE_THREAD_LIBS_INIT})
