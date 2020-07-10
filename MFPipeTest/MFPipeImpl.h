#ifndef MF_PIPEIMPL_H_
#define MF_PIPEIMPL_H_

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <iomanip>
#include <ctime>
#include <deque>
#include <thread>
#include <mutex>

#include "MFTypes.h"
#include "MFPipe.h"

class PipeReader
{
public:
	PipeReader(const std::string &pipeId,
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

	~PipeReader()
	{
		if (isRunning)
			stop();
	}

	void start()
	{
		isRunning = true;
		thread.reset(new std::thread(&PipeReader::run, this, std::cref(pipeId), dataBuffer, messageBuffer));
	}

	void stop()
	{
		{
			std::unique_lock<std::timed_mutex> lock(mutex);
			isRunning = false;
		}
		thread->join();
	}

	void run(const std::string &pipeId,
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
				fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
				if (fd == -1)
				{
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
					case Parser::State::BUFFER_READY:
					{
						const auto data = parser.getData();

						MF_BUFFER buffer;
						dataBuffer->push_back(std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data)));
						parser.reset();
						doRead = false;
						break;
					}
					case Parser::State::FRAME_READY:
					{
						const auto data = parser.getData();

						MF_FRAME frame;
						dataBuffer->push_back(std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data)));
						parser.reset();
						doRead = false;
						break;
					}
					case Parser::State::MESSAGE_READY:
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

private:
	volatile bool isRunning;
	std::string pipeId;
	int32_t fd;
	size_t maxBuffers;

	std::unique_ptr<std::thread> thread;
	std::timed_mutex mutex;

	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer;
	std::shared_ptr<std::deque<Message>> messageBuffer;
	Parser parser;
};

class PipeWriter
{
public:
	PipeWriter(const std::string &pipeId,
			   std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			   std::shared_ptr<std::deque<Message>> messageBuffer)
		: isRunning(false),
		  pipeId(pipeId),
		  fd(-1),
		  dataBuffer(dataBuffer),
		  messageBuffer(messageBuffer)
	{}

	~PipeWriter()
	{
		if (isRunning)
			stop();
	}

	void start()
	{
		isRunning = true;
		thread.reset(new std::thread(&PipeWriter::run, this, std::cref(pipeId), dataBuffer, messageBuffer));
	}

	void stop()
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

	void run(const std::string &pipeId,
			 std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			 std::shared_ptr<std::deque<Message>> messageBuffer)
	{
		std::cout << "WRITER started" << std::endl;

		while (isRunning)
		{
			if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
				continue;

			if (fd == -1)
			{
				fd = open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);
				if (fd == -1)
				{
					mutex.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				else
				{
					std::cout << "WRITER opened pipe " << fd << std::endl;
				}
			}

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
					auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
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
					auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
					if (bytes != -1)
						bytesWritten += bytes;
				} while (bytesWritten < data.size());

				mutex.unlock();
				continue;
			}
		}

		close(fd);
	}

private:
	volatile bool isRunning;
	std::string pipeId;
	int32_t fd;

	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer;
	std::shared_ptr<std::deque<Message>> messageBuffer;

	std::unique_ptr<std::thread> thread;
	std::timed_mutex mutex;
};

class MFPipeImpl: public MFPipe
{
public:

	HRESULT PipeInfoGet(
		/*[out]*/ std::string *pStrPipeName,
		/*[in]*/ const std::string &strChannel,
		MF_PIPE_INFO* _pPipeInfo) override
	{
		return E_NOTIMPL;
	}

	HRESULT PipeCreate(
		/*[in]*/ const std::string &strPipeID,
		/*[in]*/ const std::string &strHints) override
	{
		if (strPipeID.empty())
		{
			std::cout << "Can't create pipe with empty name." << std::endl;
			return E_INVALIDARG;
		}

		remove(strPipeID.c_str());

		if (mkfifo(strPipeID.c_str(), 0777) != 0)
		{
			std::cout << "Failed to create pipe. ERRNO: " << errno << std::endl;
			return E_INVALIDARG;
		} else {
			std::cout << "Pipe " << strPipeID << " created successfully." << std::endl;
		}

		pipeId = strPipeID;

		return S_OK;
	}

	HRESULT PipeOpen(
		/*[in]*/ const std::string &strPipeID,
		/*[in]*/ int _nMaxBuffers,
		/*[in]*/ const std::string &strHints) override
	{
		if (strPipeID.empty())
		{
			std::cout << "Can't open pipe with empty name." << std::endl;
			return E_INVALIDARG;
		}

		pipeId = strPipeID;

		if (strHints.find("R") != std::string::npos)
		{
			readDataBuffer = std::make_shared<std::deque<std::shared_ptr<MF_BASE_TYPE>>>();
			readMessageBuffer = std::make_shared<std::deque<Message>>();
			reader = std::make_unique<PipeReader>(pipeId, _nMaxBuffers, readDataBuffer, readMessageBuffer);
			reader->start();
		}
		if (strHints.find("W") != std::string::npos)
		{
			writeDataBuffer = std::make_shared<std::deque<std::shared_ptr<MF_BASE_TYPE>>>();
			writeMessageBuffer = std::make_shared<std::deque<Message>>();
			writer = std::make_unique<PipeWriter>(pipeId, writeDataBuffer, writeMessageBuffer);
			writer->start();
		}

		return S_OK;
	}

	HRESULT PipePut(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ const std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		if (!writeMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to add buffer to write queue." << std::endl;
			return S_FALSE;
		}

		// TODO check buffer size

		writeDataBuffer->push_back(pBufferOrFrame);
		writeMutex.unlock();

		return S_OK;
	}

	HRESULT PipeGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!readMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to get buffer from read queue" << std::endl;
			return S_FALSE;
		}

		do
		{
			if (readDataBuffer->empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			pBufferOrFrame = readDataBuffer->front();
			readDataBuffer->pop_front();
			readMutex.unlock();
			return S_OK;
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs));

		return S_FALSE;
	}

	HRESULT PipePeek(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ int _nIndex,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		return E_NOTIMPL;
	}

	HRESULT PipeMessagePut(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ const std::string &strEventName,
		/*[in]*/ const std::string &strEventParam,
		/*[in]*/ int _nMaxWaitMs) override
	{
		if (!writeMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to add message to write queue" << std::endl;
			return S_FALSE;
		}

		// TODO check buffer size

		writeMessageBuffer->push_back({ strEventName, strEventParam });
		writeMutex.unlock();

		return S_OK;
	}

	HRESULT PipeMessageGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::string *pStrEventName,
		/*[out]*/ std::string *pStrEventParam,
		/*[in]*/ int _nMaxWaitMs) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!readMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to get message from read queue" << std::endl;
			return S_FALSE;
		}

		do
		{
			if (readMessageBuffer->empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			Message mes = readMessageBuffer->front();
			readMessageBuffer->pop_front();
			*pStrEventName = mes.name;
			*pStrEventParam = mes.param;
			readMutex.unlock();
			return S_OK;
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs));

		return S_FALSE;
	}

	HRESULT PipeFlush( /*[in]*/ const std::string &strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) override
	{
		return E_NOTIMPL;
	}

	HRESULT PipeClose() override
	{
		return E_NOTIMPL;
	}

private:
	std::string pipeId;
	MF_PIPE_INFO pipeInfo;

	std::timed_mutex readMutex;
	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> readDataBuffer;
	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> writeDataBuffer;

	std::timed_mutex writeMutex;
	std::shared_ptr<std::deque<Message>> readMessageBuffer;
	std::shared_ptr<std::deque<Message>> writeMessageBuffer;

	std::unique_ptr<PipeReader> reader;
	std::unique_ptr<PipeWriter> writer;

	Parser parser;
};

#endif
