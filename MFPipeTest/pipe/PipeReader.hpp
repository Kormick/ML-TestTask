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
			   const std::string &pipeId,
			   size_t maxBuffers,
			   std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			   std::shared_ptr<std::deque<Message>> messageBuffer);

	~PipeReader();

	void start();
	void stop();

	void run(const std::string &pipeId,
			 std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			 std::shared_ptr<std::deque<Message>> messageBuffer);

private:
	volatile bool isRunning;
	std::string pipeId;
	int32_t fd;
	size_t maxBuffers;

	std::unique_ptr<std::thread> thread;
	std::timed_mutex mutex;

	std::shared_ptr<IoInterface> io;
	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer;
	std::shared_ptr<std::deque<Message>> messageBuffer;
	PipeParser parser;
};

#endif // PIPEREADER_HPP
