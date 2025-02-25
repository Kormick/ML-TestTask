#include "PipeReader.hpp"

#include "fcntl.h"
#include "unistd.h"

PipeReader::PipeReader(std::shared_ptr<IoInterface> io,
					   size_t maxBuffers,
					   std::shared_ptr<DataBuffer> dataBuffer)
	: isRunning(false),
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
	thread.reset(new std::thread(&PipeReader::run, this, dataBuffer));
}

void PipeReader::stop()
{
	isRunning = false;
	if (thread->joinable())
		thread->join();
}

void PipeReader::run(std::shared_ptr<DataBuffer> dataBuffer)
{
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

		while (true)
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
					const auto ch = parser.getChannel();
					const auto data = parser.getData();

					MF_BUFFER buf;
					dataBuffer->data.push_back({ ch, std::shared_ptr<MF_BASE_TYPE>(buf.deserialize(data)) });
					parser.reset();
					break;
				}
				case PipeParser::State::FRAME_READY:
				{
					const auto ch = parser.getChannel();
					const auto data = parser.getData();

					MF_FRAME frame;
					dataBuffer->data.push_back({ ch, std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data)) });
					parser.reset();
					break;
				}
				case PipeParser::State::MESSAGE_READY:
				{
					const auto ch = parser.getChannel();
					const auto data = parser.getData();

					Message message;
					dataBuffer->messages.push_back({ ch, std::make_shared<Message>(message.deserialize(data)) });
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
