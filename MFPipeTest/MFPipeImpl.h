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
			   size_t maxBuffers)
		: isRunning(false),
		  pipeId(pipeId),
		  maxBuffers(maxBuffers)
	{}

	void start()
	{
		isRunning = true;
		thread.reset(new std::thread(&PipeReader::run, this, std::cref(pipeId),
									 std::ref(dataBuffer), std::ref(messageBuffer)));
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
			 std::deque<std::shared_ptr<MF_BASE_TYPE>> &dataBuffer,
			 std::deque<Message> &messageBuffer)
	{
		while (isRunning)
		{
			if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
				continue;

			if (dataBuffer.size() >= maxBuffers || messageBuffer.size() >= maxBuffers)
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
						dataBuffer.push_back(std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data)));
						parser.reset();
						break;
					}
					case Parser::State::FRAME_READY:
					{
						const auto data = parser.getData();

						MF_FRAME frame;
						dataBuffer.push_back(std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data)));
						parser.reset();
						break;
					}
					case Parser::State::MESSAGE_READY:
					{
						const auto data = parser.getData();

						Message message;
						messageBuffer.push_back(message.deserialize(data));
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

	std::deque<std::shared_ptr<MF_BASE_TYPE>> dataBuffer;
	std::deque<Message> messageBuffer;
	Parser parser;
};

class PipeWriter
{
public:
	PipeWriter(const std::string &pipeId)
		: isRunning(false),
		  pipeId(pipeId)
	{}

	void start()
	{
		isRunning = true;
		thread.reset(new std::thread(&PipeWriter::run, this, std::cref(pipeId),
									 std::ref(dataBuffer), std::ref(messageBuffer)));
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
			 std::deque<std::shared_ptr<MF_BASE_TYPE>> &dataBuffer,
			 std::deque<Message> &messageBuffer)
	{
		while (isRunning)
		{
			if (!mutex.try_lock_for(std::chrono::milliseconds(10)))
				continue;

			if (dataBuffer.empty() && messageBuffer.empty())
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

			if (!dataBuffer.empty())
			{
				auto dataPtr = dataBuffer.front();
				dataBuffer.pop_front();

				const auto data = dataPtr->serialize();
				size_t bytesWritten = 0;

				while (bytesWritten < data.size())
				{
					auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
					if (bytes != -1)
						bytesWritten += bytes;
				}
			}
			else if (!messageBuffer.empty())
			{
				auto dataPtr = messageBuffer.front();
				messageBuffer.pop_front();

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

	std::unique_ptr<std::thread> thread;
	std::timed_mutex mutex;

	std::deque<std::shared_ptr<MF_BASE_TYPE>> dataBuffer;
	std::deque<Message> messageBuffer;
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

		}

		return S_OK;
	}

	HRESULT PipePut(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ const std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		time_t startTime = time(nullptr);
		size_t bytesWritten = 0;
		const auto data = pBufferOrFrame->serialize();

		std::cout << "Writing " << data.size() << " bytes to pipe" << std::endl;

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

	HRESULT PipeGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		time_t startTime = time(nullptr);

		while (time(nullptr) < startTime + _nMaxWaitMs)
		{
			int32_t fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
			if (fd == -1)
			{
				std::cout << "Failed to open on read at " << time(nullptr) << std::endl;
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

				if (parser.getState() == Parser::State::BUFFER_READY)
				{
					const auto data = parser.getData();

					MF_BUFFER buffer;
					pBufferOrFrame = std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data));

					parser.reset();

					return S_OK;
				}
				else if (parser.getState() == Parser::State::FRAME_READY)
				{
					const auto data = parser.getData();

					MF_FRAME frame;
					pBufferOrFrame = std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data));

					parser.reset();

					return S_OK;
				}
			}
		}

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

			bool readingEventName = true;
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

	std::deque<std::shared_ptr<MF_BASE_TYPE>> dataBuffer;
	std::deque<Message> messageBuffer;

	Parser parser;
};

#endif
