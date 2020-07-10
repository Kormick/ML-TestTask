project(MFPipe_Test)

find_package(Threads)

set(SOURCES
	unittest_mfpipe.cpp
	pipe/PipeParser.cpp
	pipe/PipeReader.cpp
	pipe/PipeWriter.cpp
)

set(HEADERS
	MFPipe.h
	MFPipeImpl.h
	MFTypes.h
	pipe/PipeParser.hpp
	pipe/PipeReader.hpp
	pipe/PipeWriter.hpp

	tests/Parser.hpp
	tests/Pipe.hpp
)

include_directories(
	.
	pipe
	tests
)

add_executable(MFPipe_Test ${SOURCES} ${HEADERS})

target_link_libraries(MFPipe_Test ${CMAKE_THREAD_LIBS_INIT})
