#include "PipeWriter.hpp"

#include "fcntl.h"
#include "unistd.h"

PipeWriter::PipeWriter(std::shared_ptr<IoInterface> io,
					   const std::string &pipeId,
					   std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
					   std::shared_ptr<std::deque<Message>> messageBuffer)
	: isRunning(false),
	  pipeId(pipeId),
	  fd(-1),
	  io(io),
	  dataBuffer(dataBuffer),
	  messageBuffer(messageBuffer)
{}

PipeWriter::~PipeWriter()
{
	if (isRunning)
		stop();
}

void PipeWriter::start()
{
	isRunning = true;
	thread.reset(new std::thread(&PipeWriter::run, this, std::cref(pipeId), dataBuffer, messageBuffer));
}

void PipeWriter::stop()
{
	while (true)
	{
		if (dataBuffer->empty() && messageBuffer->empty())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	isRunning = false;
	thread->join();
}

void PipeWriter::run(const std::string &pipeId,
		 std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
		 std::shared_ptr<std::deque<Message>> messageBuffer)
{
	std::cout << "WRITER started" << std::endl;

	while (isRunning)
	{
		if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
			continue;

//		if (fd == -1)
//		{
//			fd = open(pipeId.c_str(), O_WRONLY);
//			if (fd == -1)
//			{
//				std::cout << errno << std::endl;
//				mutex.unlock();
//				std::this_thread::sleep_for(std::chrono::milliseconds(1));
//				continue;
//			}
//			else
//			{
//				std::cout << "WRITER opened pipe " << fd << std::endl;
//			}
//		}

		if (dataBuffer->empty() && messageBuffer->empty())
		{
			mutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		if (!dataBuffer->empty())
		{
			size_t bytesWritten = 0;
			const auto dataPtr = dataBuffer->front();
			dataBuffer->pop_front();
			const auto data = dataPtr->serialize();

			do
			{
//				auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
				auto bytes = io->write(data.data() + bytesWritten, data.size() - bytesWritten);
				if (bytes != -1)
					bytesWritten += bytes;
			} while (bytesWritten < data.size());

			mutex.unlock();
			continue;
		}

		if (!messageBuffer->empty())
		{
			size_t bytesWritten = 0;
			const auto dataPtr = messageBuffer->front();
			messageBuffer->pop_front();
			const auto data = dataPtr.serialize();

			do
			{
//				auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
				auto bytes = io->write(data.data() + bytesWritten, data.size() - bytesWritten);
				if (bytes != -1)
					bytesWritten += bytes;
			} while (bytesWritten < data.size());

			mutex.unlock();
			continue;
		}
	}
}
