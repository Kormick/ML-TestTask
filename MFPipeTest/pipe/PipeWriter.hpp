#ifndef PIPEWRITER_HPP
#define PIPEWRITER_HPP

#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "IoInterface.hpp"
#include "MFTypes.h"

class PipeWriter
{
public:
	PipeWriter(std::shared_ptr<IoInterface> io,
			   std::shared_ptr<DataBuffer> dataBuffer);

	~PipeWriter();

	void start();
	void stop();
	void run(std::shared_ptr<DataBuffer> dataBuffer);

private:
	volatile bool isRunning;
	std::shared_ptr<IoInterface> io;
	std::shared_ptr<DataBuffer> dataBuffer;
	std::unique_ptr<std::thread> thread;
};

#endif // PIPEWRITER_HPP
