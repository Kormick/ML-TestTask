project(MFPipe_Test)

find_package(Threads)

set(SOURCES
	unittest_mfpipe.cpp
)

set(HEADERS
	MFPipe.h
	MFPipeImpl.h
	MFTypes.h
)

add_executable(MFPipe_Test ${SOURCES} ${HEADERS})

target_link_libraries(MFPipe_Test ${CMAKE_THREAD_LIBS_INIT})
