#include "PipeReader.hpp"

#include "fcntl.h"
#include "unistd.h"

PipeReader::PipeReader(std::shared_ptr<IoInterface> io,
					   const std::string &pipeId,
					   size_t maxBuffers,
					   std::shared_ptr<DataBuffer> dataBuffer)
	: isRunning(false),
	  pipeId(pipeId),
	  fd(-1),
	  maxBuffers(maxBuffers),
	  io(io),
	  dataBuffer(dataBuffer)
{}

PipeReader::~PipeReader()
{
	if (isRunning)
		stop();
}

void PipeReader::start()
{
	isRunning = true;
	thread.reset(new std::thread(&PipeReader::run, this, std::cref(pipeId), dataBuffer));
}

void PipeReader::stop()
{
	isRunning = false;
	if (thread->joinable())
		thread->join();
}

void PipeReader::run(const std::string &pipeId, std::shared_ptr<DataBuffer> dataBuffer)
{
	std::cout << "READER started" << std::endl;

	uint8_t buffer[512 * 1024] = { 0 };
	size_t parsedBytes = 0;
	ssize_t readBytes = 0;

	while (isRunning)
	{
		if (!dataBuffer->mutex.try_lock_for(std::chrono::milliseconds(10)))
			continue;

		if (dataBuffer->data.size() >= maxBuffers || dataBuffer->messages.size() >= maxBuffers)
		{
			dataBuffer->mutex.unlock();
			std::this_thread::yield();
			continue;
		}

		bool doRead = true;
		while (doRead)
		{
			if (readBytes <= 0)
				readBytes = io->read(buffer, 512 * 1024);

			if (readBytes <= 0)
				break;

			parsedBytes += parser.parse(buffer + parsedBytes, readBytes - parsedBytes);

			switch (parser.getState())
			{
				case PipeParser::State::BUFFER_READY:
				{
					const auto data = parser.getData();

					MF_BUFFER buffer;
					dataBuffer->data.push_back(std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data)));
					parser.reset();
					break;
				}
				case PipeParser::State::FRAME_READY:
				{
					const auto data = parser.getData();

					MF_FRAME frame;
					dataBuffer->data.push_back(std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data)));
					parser.reset();
					break;
				}
				case PipeParser::State::MESSAGE_READY:
				{
					const auto data = parser.getData();

					Message message;
					dataBuffer->messages.push_back(message.deserialize(data));
					parser.reset();
					break;
				}
				default:
					break;
			}

			if (static_cast<size_t>(readBytes) == parsedBytes)
			{
				readBytes = 0;
				parsedBytes = 0;
				break;
			}
		}

		dataBuffer->mutex.unlock();
	}
}
