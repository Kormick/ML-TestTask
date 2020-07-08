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

		return S_OK;
	}

	HRESULT PipePut(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ const std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!mutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to write buffer" << std::endl;
			return S_FALSE;
		}

		size_t bytesWritten = 0;
		const auto data = pBufferOrFrame->serialize();

		do
		{
			int32_t fd = open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);
			if (fd == -1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
			if (bytes != -1)
				bytesWritten += bytes;

			close(fd);
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs)
				 && bytesWritten < data.size());

		mutex.unlock();

		return bytesWritten == data.size() ? S_OK : S_FALSE;
	}

	HRESULT PipeGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!mutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to read buffer" << std::endl;
			return S_FALSE;
		}

		do
		{
			int32_t fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
			if (fd == -1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			while (true)
			{
				uint8_t byte = 0;
				std::vector<uint8_t> data;

				if (read(fd, &byte, sizeof(byte)) == -1)
					break;

				data.push_back(byte);
				parser.parse(data, 0);

				switch (parser.getState())
				{
					case Parser::State::BUFFER_READY:
					{
						const auto data = parser.getData();

						MF_BUFFER buffer;
						pBufferOrFrame = std::shared_ptr<MF_BASE_TYPE>(buffer.deserialize(data));
						parser.reset();
						mutex.unlock();
						return S_OK;
					}
					case Parser::State::FRAME_READY:
					{
						const auto data = parser.getData();

						MF_FRAME frame;
						pBufferOrFrame = std::shared_ptr<MF_BASE_TYPE>(frame.deserialize(data));
						parser.reset();
						mutex.unlock();
						return S_OK;
					}
					default:
						break;
				}
			}
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs));

		mutex.unlock();

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
		auto start = std::chrono::steady_clock::now();

		if (!mutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to write message" << std::endl;
			return S_FALSE;
		}

		size_t bytesWritten = 0;
		Message mes;
		mes.name = strEventName;
		mes.param = strEventParam;
		const auto data = mes.serialize();

		do
		{
			int32_t fd = open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);
			if (fd == -1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			auto bytes = write(fd, data.data() + bytesWritten, data.size() - bytesWritten);
			if (bytes != -1)
				bytesWritten += bytes;

			close(fd);
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs)
				 && bytesWritten < data.size());

		mutex.unlock();

		return bytesWritten == data.size() ? S_OK : S_FALSE;
	}

	HRESULT PipeMessageGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::string *pStrEventName,
		/*[out]*/ std::string *pStrEventParam,
		/*[in]*/ int _nMaxWaitMs) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!mutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to read message" << std::endl;
			return S_FALSE;
		}

		do
		{
			int32_t fd = open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
			if (fd == -1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			while (true)
			{
				uint8_t byte = 0;
				std::vector<uint8_t> data;

				if (read(fd, &byte, sizeof(byte)) == -1)
					break;

				data.push_back(byte);
				parser.parse(data, 0);

				switch (parser.getState())
				{
					case Parser::State::MESSAGE_READY:
					{
						const auto data = parser.getData();

						Message mes;
						mes = mes.deserialize(data);
						parser.reset();

						*pStrEventName = mes.name;
						*pStrEventParam = mes.param;

						mutex.unlock();
						return S_OK;
					}
					default:
						break;
				}
			}
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs));

		mutex.unlock();
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

	std::timed_mutex mutex;

	std::deque<std::shared_ptr<MF_BASE_TYPE>> dataBuffer;
	std::deque<Message> messageBuffer;

	Parser parser;
};

#endif
