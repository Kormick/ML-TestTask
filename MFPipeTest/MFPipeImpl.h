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
#include "udp/UnixIoUdp.hpp"
#else
#include "udp/WinIoUdp.hpp"
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
	~MFPipeImpl() override
	{
		PipeClose();
	}

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

		if (strPipeID.find("udp") != std::string::npos)
			io = std::make_shared<IoUdp>();
		else
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
			/*[in]*/ const std::string &strHints,
			/*[in]*/ int _nMaxWaitMs = 10000) override
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
			{
				if (strPipeID.find("udp") != std::string::npos)
					io = std::make_shared<IoUdp>();
				else
					io = std::make_shared<IoPipe>();
			}

			if (!io->open(pipeId, IoInterface::Mode::READ, _nMaxWaitMs))
			{
				std::cout << "Failed to open pipe on read." << std::endl;
				return MF_HRESULT::RES_FALSE;
			}

			readDataBuffer = std::make_shared<DataBuffer>();
			reader = std::make_unique<PipeReader>(io, pipeId, _nMaxBuffers, readDataBuffer);
			reader->start();
		}
		if (strHints.find("W") != std::string::npos)
		{
			if (!io->open(pipeId, IoInterface::Mode::WRITE, _nMaxWaitMs))
			{
				std::cout << "Failed to open pipe on write." << std::endl;
				return MF_HRESULT::RES_FALSE;
			}

			maxBuffers = _nMaxBuffers;
			writeDataBuffer = std::make_shared<DataBuffer>();
			writer = std::make_unique<PipeWriter>(io, pipeId, writeDataBuffer);
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
		const auto start = std::chrono::steady_clock::now();
		const auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

		do
		{
			if (!writeDataBuffer->mutex.try_lock_until(end))
				break;

			if (writeDataBuffer->data.size() >= maxBuffers)
			{
				writeDataBuffer->mutex.unlock();
				std::this_thread::yield();
				continue;
			}

			writeDataBuffer->data.push_back(pBufferOrFrame);
			writeDataBuffer->mutex.unlock();
			return MF_HRESULT::RES_OK;
		} while (std::chrono::steady_clock::now() < end);

		std::cout << "Timeout on adding buffer to write queue" << std::endl;
		return MF_HRESULT::RES_FALSE;
	}

	MF_HRESULT PipeGet(
			/*[in]*/ const std::string &strChannel,
			/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override
	{
		auto start = std::chrono::steady_clock::now();
		auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

		do
		{
			if (!readDataBuffer->mutex.try_lock_until(end))
				break;

			if (readDataBuffer->data.empty())
			{
				readDataBuffer->mutex.unlock();
				std::this_thread::yield();
				continue;
			}

			pBufferOrFrame = readDataBuffer->data.front();
			readDataBuffer->data.pop_front();
			readDataBuffer->mutex.unlock();
			return MF_HRESULT::RES_OK;
		} while (std::chrono::steady_clock::now() < end);

		std::cout << "Timeout on getting buffer from read queue" << std::endl;
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
		const auto start = std::chrono::steady_clock::now();
		const auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

		do
		{
			if (!writeDataBuffer->mutex.try_lock_until(end))
				break;

			if (writeDataBuffer->messages.size() >= maxBuffers)
			{
				writeDataBuffer->mutex.unlock();
				std::this_thread::yield();
				continue;
			}

			writeDataBuffer->messages.push_back({ strEventName, strEventParam });
			writeDataBuffer->mutex.unlock();
			return MF_HRESULT::RES_OK;
		} while (std::chrono::steady_clock::now() < end);

		std::cout << "Timeout on adding message to write queue" << std::endl;
		return MF_HRESULT::RES_FALSE;
	}

	MF_HRESULT PipeMessageGet(
			/*[in]*/ const std::string &strChannel,
			/*[out]*/ std::string *pStrEventName,
			/*[out]*/ std::string *pStrEventParam,
			/*[in]*/ int _nMaxWaitMs) override
	{
		auto start = std::chrono::steady_clock::now();
		auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

		do
		{
			if (!readDataBuffer->mutex.try_lock_until(end))
				break;

			if (readDataBuffer->messages.empty())
			{
				readDataBuffer->mutex.unlock();
				std::this_thread::yield();
				continue;
			}

			Message mes = readDataBuffer->messages.front();
			readDataBuffer->messages.pop_front();
			*pStrEventName = mes.name;
			*pStrEventParam = mes.param;
			readDataBuffer->mutex.unlock();
			return MF_HRESULT::RES_OK;
		} while (std::chrono::steady_clock::now() < end);

		std::cout << "Timeout on getting message from read queue" << std::endl;
		return MF_HRESULT::RES_FALSE;
	}

	MF_HRESULT PipeFlush( /*[in]*/ const std::string &strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) override
	{
		return MF_HRESULT::NOTIMPL;
	}

	MF_HRESULT PipeClose() override
	{
		if (reader)
			reader->stop();
		if (writer)
			writer->stop();

		return io->close() ? MF_HRESULT::RES_OK : MF_HRESULT::RES_FALSE;
	}

private:
	std::string pipeId;
	int32_t maxBuffers;
	MF_PIPE_INFO pipeInfo;

	std::shared_ptr<IoInterface> io;

	std::shared_ptr<DataBuffer> readDataBuffer;
	std::shared_ptr<DataBuffer> writeDataBuffer;

	std::unique_ptr<PipeReader> reader;
	std::unique_ptr<PipeWriter> writer;
};

#endif
