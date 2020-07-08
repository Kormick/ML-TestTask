project(MFPipe_Test)

set(SOURCES
	unittest_mfpipe.cpp
)

set(HEADERS
	MFPipe.h
	MFPipeImpl.h
	MFTypes.h
)

add_executable(MFPipe_Test ${SOURCES} ${HEADERS})
