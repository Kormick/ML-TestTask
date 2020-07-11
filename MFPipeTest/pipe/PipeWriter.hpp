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
			   const std::string &pipeId,
			   std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			   std::shared_ptr<std::deque<Message>> messageBuffer);

	~PipeWriter();

	void start();
	void stop();
	void run(const std::string &pipeId,
			 std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			 std::shared_ptr<std::deque<Message>> messageBuffer);

private:
	volatile bool isRunning;
	std::string pipeId;
	int32_t fd;

	std::shared_ptr<IoInterface> io;
	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer;
	std::shared_ptr<std::deque<Message>> messageBuffer;

	std::unique_ptr<std::thread> thread;
	std::timed_mutex mutex;
};

#endif // PIPEWRITER_HPP
