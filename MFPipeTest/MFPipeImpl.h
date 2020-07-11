#ifndef MF_PIPEIMPL_H_
#define MF_PIPEIMPL_H_

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstdint>
#include <ctime>
#include <deque>
#include <mutex>

#include "sys/stat.h"
#include "unistd.h"

#ifdef unix
#include "pipe/UnixIoPipe.hpp"
#else
#include "pipe/WinIoPipe.hpp"
#endif

#include "IoInterface.hpp"
#include "MFTypes.h"
#include "MFPipe.h"
#include "PipeReader.hpp"
#include "PipeWriter.hpp"

class MFPipeImpl: public MFPipe
{
public:
	MF_HRESULT PipeInfoGet(
			/*[out]*/ std::string *pStrPipeName,
			/*[in]*/ const std::string &strChannel,
			MF_PIPE_INFO* _pPipeInfo) override
	{
		return MF_HRESULT::NOTIMPL;
	}

	MF_HRESULT PipeCreate(
			/*[in]*/ const std::string &strPipeID,
			/*[in]*/ const std::string &strHints) override
	{
		if (strPipeID.empty())
		{
			std::cout << "Can't create pipe with empty name." << std::endl;
			return MF_HRESULT::INVALIDARG;
		}

		io = std::make_shared<IoPipe>();

		if (io->create(strPipeID))
		{
			std::cout << "Pipe " << strPipeID << " created successfully." << std::endl;
		}
		else
		{
			std::cout << "Failed to create pipe. ERRNO: " << errno << std::endl;
			return MF_HRESULT::RES_FALSE;
		}

		pipeId = strPipeID;
		return MF_HRESULT::RES_OK;
	}

	MF_HRESULT PipeOpen(
			/*[in]*/ const std::string &strPipeID,
			/*[in]*/ int _nMaxBuffers,
			/*[in]*/ const std::string &strHints) override
	{
		if (strPipeID.empty())
		{
			std::cout << "Can't open pipe with empty name." << std::endl;
			return MF_HRESULT::INVALIDARG;
		}

		pipeId = strPipeID;

		if (strHints.find("R") != std::string::npos)
		{
			if (!io)
				io = std::make_shared<IoPipe>();

			if (!io->open(pipeId, IoInterface::Mode::READ))
			{
				std::cout << "Failed to open pipe on read." << std::endl;
				return MF_HRESULT::RES_FALSE;
			}

			readDataBuffer = std::make_shared<std::deque<std::shared_ptr<MF_BASE_TYPE>>>();
			readMessageBuffer = std::make_shared<std::deque<Message>>();
			reader = std::make_unique<PipeReader>(io, pipeId, _nMaxBuffers, readDataBuffer, readMessageBuffer);
			reader->start();
		}
		if (strHints.find("W") != std::string::npos)
		{
			if (!io->open(pipeId, IoInterface::Mode::WRITE))
			{
				std::cout << "Failed to open pipe on write." << std::endl;
				return MF_HRESULT::RES_FALSE;
			}

			writeDataBuffer = std::make_shared<std::deque<std::shared_ptr<MF_BASE_TYPE>>>();
			writeMessageBuffer = std::make_shared<std::deque<Message>>();
			writer = std::make_unique<PipeWriter>(io, pipeId, writeDataBuffer, writeMessageBuffer);
			writer->start();
		}

		return MF_HRESULT::RES_OK;
	}

	MF_HRESULT PipePut(
			/*[in]*/ const std::string &strChannel,
			/*[in]*/ const std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override
	{
		if (!writeMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to add buffer to write queue." << std::endl;
			return MF_HRESULT::RES_FALSE;
		}

		// TODO check buffer size

		writeDataBuffer->push_back(pBufferOrFrame);
		writeMutex.unlock();

		return MF_HRESULT::RES_OK;
	}

	MF_HRESULT PipeGet(
			/*[in]*/ const std::string &strChannel,
			/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!readMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to get buffer from read queue" << std::endl;
			return MF_HRESULT::RES_FALSE;
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
			return MF_HRESULT::RES_OK;
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs));

		return MF_HRESULT::RES_FALSE;
	}

	MF_HRESULT PipePeek(
			/*[in]*/ const std::string &strChannel,
			/*[in]*/ int _nIndex,
			/*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override
	{
		return MF_HRESULT::NOTIMPL;
	}

	MF_HRESULT PipeMessagePut(
			/*[in]*/ const std::string &strChannel,
			/*[in]*/ const std::string &strEventName,
			/*[in]*/ const std::string &strEventParam,
			/*[in]*/ int _nMaxWaitMs) override
	{
		if (!writeMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to add message to write queue" << std::endl;
			return MF_HRESULT::RES_FALSE;
		}

		// TODO check buffer size

		writeMessageBuffer->push_back({ strEventName, strEventParam });
		writeMutex.unlock();

		return MF_HRESULT::RES_OK;
	}

	MF_HRESULT PipeMessageGet(
			/*[in]*/ const std::string &strChannel,
			/*[out]*/ std::string *pStrEventName,
			/*[out]*/ std::string *pStrEventParam,
			/*[in]*/ int _nMaxWaitMs) override
	{
		auto start = std::chrono::steady_clock::now();

		if (!readMutex.try_lock_for(std::chrono::milliseconds(_nMaxWaitMs)))
		{
			std::cout << "Failed to get message from read queue" << std::endl;
			return MF_HRESULT::RES_FALSE;
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
			return MF_HRESULT::RES_OK;
		} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(_nMaxWaitMs));

		return MF_HRESULT::RES_FALSE;
	}

	MF_HRESULT PipeFlush( /*[in]*/ const std::string &strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) override
	{
		return MF_HRESULT::NOTIMPL;
	}

	MF_HRESULT PipeClose() override
	{
		return MF_HRESULT::NOTIMPL;
	}

private:
	std::string pipeId;
	MF_PIPE_INFO pipeInfo;

	std::shared_ptr<IoInterface> io;

	std::timed_mutex readMutex;
	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> readDataBuffer;
	std::shared_ptr<std::deque<std::shared_ptr<MF_BASE_TYPE>>> writeDataBuffer;

	std::timed_mutex writeMutex;
	std::shared_ptr<std::deque<Message>> readMessageBuffer;
	std::shared_ptr<std::deque<Message>> writeMessageBuffer;

	std::unique_ptr<PipeReader> reader;
	std::unique_ptr<PipeWriter> writer;
};

#endif
