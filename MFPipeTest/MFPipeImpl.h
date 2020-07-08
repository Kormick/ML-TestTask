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
		  maxBuffers(maxBuffers),
		  dataBuffer(dataBuffer),
		  messageBuffer(messageBuffer)
	{}

	~PipeReader()
	{
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
		while (isRunning)
		{
			if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
				continue;

			if (dataBuffer->size() >= maxBuffers || messageBuffer->size() >= maxBuffers)
			{
				mutex.unlock();
				continue;
			}

			int32_t fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
			if (fd == -1)
			{
				mutex.unlock();
				continue;
			}

			while (true)
			{
				uint8_t byte;
				std::vector<uint8_t> data;

				if (read(fd, &byte, sizeof(uint8_t)) == -1)
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
						break;
					}
					case Parser::State::FRAME_READY:
					{
						const auto data = parser.getData();

						MF_FRAME frame;
						dataBuffer->push_back(std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data)));
						parser.reset();
						break;
					}
					case Parser::State::MESSAGE_READY:
					{
						const auto data = parser.getData();

						Message message;
						messageBuffer->push_back(message.deserialize(data));
						parser.reset();
						break;
					}
					default:
						break;
				}
			}

			close(fd);
		}
	}

private:
	volatile bool isRunning;
	std::string pipeId;
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
		  dataBuffer(dataBuffer),
		  messageBuffer(messageBuffer)
	{}

	~PipeWriter()
	{
		stop();
	}

	void start()
	{
		isRunning = true;
		thread.reset(new std::thread(&PipeWriter::run, this, std::cref(pipeId), dataBuffer, messageBuffer));
	}

	void stop()
	{
		bool wait = true;
		while (wait)
		{
			mutex.lock();

			if (dataBuffer->empty() && messageBuffer->empty())
				break;

			mutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

//		{
//			std::unique_lock<std::timed_mutex> lock(mutex);
//			isRunning = false;
//		}
		thread->join();
	}

	void run(const std::string &pipeId,
			 std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> dataBuffer,
			 std::shared_ptr<std::deque<Message>> messageBuffer)
	{
		while (isRunning)
		{
			if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
				continue;

			if (dataBuffer->empty() && messageBuffer->empty())
			{
				mutex.unlock();
				continue;
			}

			int32_t fd = open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);
			if (fd == -1)
			{
				mutex.unlock();
				continue;
			}

			if (!dataBuffer->empty())
			{
				auto dataPtr = dataBuffer->front();
				dataBuffer->pop_front();

				const auto data = dataPtr->serialize();
				size_t bytesWritten = 0;

				std::cout << "Trying to write " << data.size() << " bytes" << std::endl;

				while (bytesWritten < data.size())
				{
					auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
					if (bytes != -1)
						bytesWritten += bytes;
				}
			}
			else if (!messageBuffer->empty())
			{
				auto dataPtr = messageBuffer->front();
				messageBuffer->pop_front();

				const auto data = dataPtr.serialize();
				size_t bytesWritten = 0;

				while (bytesWritten < data.size())
				{
					auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
					if (bytes != -1)
						bytesWritten += bytes;
				}
			}

			close(fd);
		}
	}

private:
	volatile bool isRunning;
	std::string pipeId;

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


//		time_t startTime = time(nullptr);
//		size_t bytesWritten = 0;
//		const auto data = pBufferOrFrame->serialize();

//		std::cout << "Writing " << data.size() << " bytes to pipe" << std::endl;

//		while (time(nullptr) < startTime + _nMaxWaitMs && bytesWritten < data.size())
//		{
//			int32_t fd = open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);
//			if (fd == -1)
//			{
//				usleep(100);
//				continue;
//			}

//			auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
//			if (bytes != -1)
//				bytesWritten += bytes;

//			close(fd);
//		}

//		return bytesWritten == data.size() ? S_OK : S_FALSE;
	}

	HRESULT PipeGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		time_t startTime = time(nullptr);

		while(time(nullptr) < startTime + _nMaxWaitMs)
		{
			if (!readMutex.try_lock_for(std::chrono::milliseconds(10)))
				continue;

			if (readDataBuffer->empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			pBufferOrFrame = readDataBuffer->front();
			readDataBuffer->pop_front();
			readMutex.unlock();
			return S_OK;
		}

		return S_FALSE;

//		if (!readMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
//		{
//			std::cout << "Failed to get buffer from read queue" << std::endl;
//			return S_FALSE;
//		}

//		if (readDataBuffer->empty())
//		{
//			std::cout << "Read queue is empty" << std::endl;
//			readMutex.unlock();
//			return S_FALSE;
//		}

//		pBufferOrFrame = readDataBuffer->front();
//		readDataBuffer->pop_front();
//		readMutex.unlock();

//		return S_OK;

		//////////////////

//		time_t startTime = time(nullptr);

//		while (time(nullptr) < startTime + _nMaxWaitMs)
//		{
//			int32_t fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
//			if (fd == -1)
//			{
//				std::cout << "Failed to open on read at " << time(nullptr) << std::endl;
//				usleep(100);
//				continue;
//			}

//			bool doRead = true;
//			while (doRead)
//			{
//				uint8_t byte;
//				std::vector<uint8_t> data;

//				if (read(fd, &byte, sizeof(uint8_t)) == -1)
//				{
//					doRead = false;
//					break;
//				}

//				data.push_back(byte);
//				parser.parse(data, 0);

//				if (parser.getState() == Parser::State::BUFFER_READY)
//				{
//					const auto data = parser.getData();

//					MF_BUFFER buffer;
//					pBufferOrFrame = std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data));

//					parser.reset();

//					return S_OK;
//				}
//				else if (parser.getState() == Parser::State::FRAME_READY)
//				{
//					const auto data = parser.getData();

//					MF_FRAME frame;
//					pBufferOrFrame = std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data));

//					parser.reset();

//					return S_OK;
//				}
//			}
//		}

//		return S_FALSE;
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
		time_t startTime = time(nullptr);
		size_t bytesWritten = 0;
		Message mes;
		mes.name = strEventName;
		mes.param = strEventParam;
		const auto data = mes.serialize();

		while (time(nullptr) < startTime + _nMaxWaitMs && bytesWritten < data.size())
		{
			int32_t fd = open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);
			if (fd == -1)
			{
				usleep(100);
				continue;
			}

			auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
			if (bytes != -1)
				bytesWritten += bytes;

			close(fd);
		}

		return bytesWritten == data.size() ? S_OK : S_FALSE;
	}

	HRESULT PipeMessageGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::string *pStrEventName,
		/*[out]*/ std::string *pStrEventParam,
		/*[in]*/ int _nMaxWaitMs) override
	{
		time_t startTime = time(nullptr);

		while(time(nullptr) < startTime + _nMaxWaitMs)
		{
			int32_t fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
			if (fd == -1)
			{
				usleep(100);
				continue;
			}

			bool doRead = true;
			while (doRead)
			{
				uint8_t byte;
				std::vector<uint8_t> data;

				if (read(fd, &byte, sizeof(uint8_t)) == -1)
				{
					doRead = false;
					break;
				}

				data.push_back(byte);
				parser.parse(data, 0);

				if (parser.getState() == Parser::State::MESSAGE_READY)
				{
					const auto data = parser.getData();

					Message mes = mes.deserialize(data);

					parser.reset();

					*pStrEventName = mes.name;
					*pStrEventParam = mes.param;

					return S_OK;
				}
			}
		}

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
