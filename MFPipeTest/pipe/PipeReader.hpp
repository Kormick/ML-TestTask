#ifndef PIPEREADER_HPP
#define PIPEREADER_HPP

#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "IoInterface.hpp"
#include "MFTypes.h"
#include "pipe/PipeParser.hpp"

class PipeReader
{
public:
	PipeReader(std::shared_ptr<IoInterface> io,
			   size_t maxBuffers,
			   std::shared_ptr<DataBuffer> dataBuffer);

	~PipeReader();

	void start();
	void stop();

	void run(std::shared_ptr<DataBuffer> dataBuffer);

private:
	volatile bool isRunning;
	size_t maxBuffers;
	std::unique_ptr<std::thread> thread;
	std::shared_ptr<IoInterface> io;
	std::shared_ptr<DataBuffer> dataBuffer;
	PipeParser parser;
};

#endif // PIPEREADER_HPP
