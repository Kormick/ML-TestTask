#include "PipeWriter.hpp"

#include "fcntl.h"
#include "unistd.h"

PipeWriter::PipeWriter(std::shared_ptr<IoInterface> io,
					   const std::string &pipeId,
                       std::shared_ptr<DataBuffer> dataBuffer)
	: isRunning(false),
	  pipeId(pipeId),
	  fd(-1),
	  io(io),
	  dataBuffer(dataBuffer)
{}

PipeWriter::~PipeWriter()
{
	if (isRunning)
		stop();
}

void PipeWriter::start()
{
	isRunning = true;
	thread.reset(new std::thread(&PipeWriter::run, this, dataBuffer));
}

void PipeWriter::stop()
{
	while (true)
	{
		if (dataBuffer->mutex.try_lock())
		{
			if (dataBuffer->data.empty() && dataBuffer->messages.empty())
				break;
		}
		dataBuffer->mutex.unlock();
		std::this_thread::yield();
	}
	isRunning = false;
	if (thread->joinable())
		thread->join();
}

void PipeWriter::run(std::shared_ptr<DataBuffer> dataBuffer)
{
	while (isRunning)
	{
		if (!dataBuffer->mutex.try_lock())
		{
			std::this_thread::yield();
			continue;
		}

		if (dataBuffer->data.empty() && dataBuffer->messages.empty())
		{
			dataBuffer->mutex.unlock();
			std::this_thread::yield();
			continue;
		}

		if (!dataBuffer->data.empty())
		{
			size_t bytesWritten = 0;
			const auto dataPtr = dataBuffer->data.front();
			dataBuffer->data.pop_front();
			const auto data = dataPtr->serialize();

			do
			{
				auto bytes = io->write(data.data() + bytesWritten, data.size() - bytesWritten);
				if (bytes != -1)
					bytesWritten += bytes;
			} while (bytesWritten < data.size());

			dataBuffer->mutex.unlock();
			continue;
		}

		if (!dataBuffer->messages.empty())
		{
			size_t bytesWritten = 0;
			const auto dataPtr = dataBuffer->messages.front();
			dataBuffer->messages.pop_front();
			const auto data = dataPtr.serialize();

			do
			{
				auto bytes = io->write(data.data() + bytesWritten, data.size() - bytesWritten);
				if (bytes != -1)
					bytesWritten += bytes;
			} while (bytesWritten < data.size());

			dataBuffer->mutex.unlock();
			continue;
		}
	}
}
