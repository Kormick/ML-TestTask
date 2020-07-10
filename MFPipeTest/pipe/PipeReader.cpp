#include "PipeReader.hpp"

#include "fcntl.h"
#include "unistd.h"

PipeReader::PipeReader(const std::string &pipeId,
		   size_t maxBuffers,
		   std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
		   std::shared_ptr<std::deque<Message>> messageBuffer)
	: isRunning(false),
	  pipeId(pipeId),
	  fd(-1),
	  maxBuffers(maxBuffers),
	  dataBuffer(dataBuffer),
	  messageBuffer(messageBuffer)
{}

PipeReader::~PipeReader()
{
	if (isRunning)
		stop();
}

void PipeReader::start()
{
	isRunning = true;
	thread.reset(new std::thread(&PipeReader::run, this, std::cref(pipeId), dataBuffer, messageBuffer));
}

void PipeReader::stop()
{
	{
		std::unique_lock<std::timed_mutex> lock(mutex);
		isRunning = false;
	}
	thread->join();
}

void PipeReader::run(const std::string &pipeId,
		 std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
		 std::shared_ptr<std::deque<Message>> messageBuffer)
{
	std::cout << "READER started" << std::endl;

	while (isRunning)
	{
		if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
			continue;

		if (dataBuffer->size() >= maxBuffers || messageBuffer->size() >= maxBuffers)
		{
			mutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		if (fd == -1)
		{
			fd = open(pipeId.c_str(), O_RDONLY);
			if (fd == -1)
			{
				std::cout << errno << std::endl;
				mutex.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			else
			{
				std::cout << "READER opened pipe " << fd << std::endl;
			}
		}

		bool doRead = true;
		while (doRead)
		{
			uint8_t byte = 0;
			std::vector<uint8_t> data;

			auto readBytes = read(fd, &byte, sizeof(uint8_t));
			if (readBytes <= 0)
				break;

			data.push_back(byte);
			parser.parse(data, 0);

			switch (parser.getState())
			{
				case PipeParser::State::BUFFER_READY:
				{
					const auto data = parser.getData();

					MF_BUFFER buffer;
					dataBuffer->push_back(std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data)));
					parser.reset();
					doRead = false;
					break;
				}
				case PipeParser::State::FRAME_READY:
				{
					const auto data = parser.getData();

					MF_FRAME frame;
					dataBuffer->push_back(std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data)));
					parser.reset();
					doRead = false;
					break;
				}
				case PipeParser::State::MESSAGE_READY:
				{
					const auto data = parser.getData();

					Message message;
					messageBuffer->push_back(message.deserialize(data));
					parser.reset();
					doRead = false;
					break;
				}
				default:
					break;
			}
		}

		mutex.unlock();
	}

	close(fd);
}
