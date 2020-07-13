#include "MFPipeImpl.h"

#ifdef unix
#include "pipe/UnixIoPipe.hpp"
#include "udp/UnixIoUdp.hpp"
#else
#include "udp/WinIoUdp.hpp"
#include "pipe/WinIoPipe.hpp"
#endif

MFPipeImpl::~MFPipeImpl()
{
	PipeClose();
}

MF_HRESULT MFPipeImpl::PipeInfoGet(
		/*[out]*/ std::string *pStrPipeName,
		/*[in]*/ const std::string &strChannel,
		MF_PIPE_INFO *_pPipeInfo)
{
	return MF_HRESULT::NOTIMPL;
}

MF_HRESULT MFPipeImpl::PipeCreate(
		/*[in]*/ const std::string &strPipeID,
		/*[in]*/ const std::string &strHints)
{
	if (strPipeID.empty())
	{
		std::cerr << "Can't create pipe with empty name." << std::endl;
		return MF_HRESULT::INVALIDARG;
	}

	if (strPipeID.find("udp") != std::string::npos)
		io = std::make_shared<IoUdp>();
	else
		io = std::make_shared<IoPipe>();

	if (!io->create(strPipeID))
	{
		std::cerr << "Failed to create pipe. ERRNO: " << errno << std::endl;
		return MF_HRESULT::RES_FALSE;
	}

	pipeId = strPipeID;
	return MF_HRESULT::RES_OK;
}

MF_HRESULT MFPipeImpl::PipeOpen(
		/*[in]*/ const std::string &strPipeID,
		/*[in]*/ int _nMaxBuffers,
		/*[in]*/ const std::string &strHints,
		/*[in]*/ int _nMaxWaitMs)
{
	if (strPipeID.empty())
	{
		std::cerr << "Can't open pipe with empty name." << std::endl;
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
			std::cerr << "Failed to open pipe on read." << std::endl;
			return MF_HRESULT::RES_FALSE;
		}

		readDataBuffer = std::make_shared<DataBuffer>();
		reader = std::make_unique<PipeReader>(io, _nMaxBuffers, readDataBuffer);
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
		writer = std::make_unique<PipeWriter>(io, writeDataBuffer);
		writer->start();
	}

	return MF_HRESULT::RES_OK;
}

MF_HRESULT MFPipeImpl::PipePut(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ const std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints)
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

		writeDataBuffer->data.push_back({ strChannel, pBufferOrFrame });
		writeDataBuffer->mutex.unlock();
		return MF_HRESULT::RES_OK;
	} while (std::chrono::steady_clock::now() < end);

	std::cerr << "Timeout on adding buffer to write queue" << std::endl;
	return MF_HRESULT::RES_FALSE;
}

MF_HRESULT MFPipeImpl::PipeGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints)
{
	auto start = std::chrono::steady_clock::now();
	auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

	do
	{
		if (!readDataBuffer->mutex.try_lock_until(end))
			break;

		auto it = readDataBuffer->data.begin();
		for (; it != readDataBuffer->data.end(); ++it)
		{
			if (it->first == strChannel)
				break;
		}

		if (it == readDataBuffer->data.end())
		{
			readDataBuffer->mutex.unlock();
			std::this_thread::yield();
			continue;
		}

		pBufferOrFrame = it->second;
		readDataBuffer->data.erase(it);
		readDataBuffer->mutex.unlock();
		return MF_HRESULT::RES_OK;
	} while (std::chrono::steady_clock::now() < end);

	std::cerr << "Timeout on getting buffer from read queue" << std::endl;
	return MF_HRESULT::RES_FALSE;
}

MF_HRESULT MFPipeImpl::PipePeek(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ int _nIndex,
		/*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
		/*[in]*/ int _nMaxWaitMs,
		/*[in]*/ const std::string &strHints)
{
	auto start = std::chrono::steady_clock::now();
	auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

	do
	{
		if (!readDataBuffer->mutex.try_lock_until(end))
			break;

		auto it = readDataBuffer->data.begin();
		auto count = 0;
		for (; it != readDataBuffer->data.end() && count <= _nIndex; ++it, ++count)
		{
			if (it->first == strChannel && count == _nIndex)
				break;
		}

		if (it == readDataBuffer->data.end())
		{
			readDataBuffer->mutex.unlock();
			std::this_thread::yield();
			continue;
		}

		pBufferOrFrame = it->second;
		readDataBuffer->mutex.unlock();
		return MF_HRESULT::RES_OK;
	} while (std::chrono::steady_clock::now() < end);

	return MF_HRESULT::RES_FALSE;
}

MF_HRESULT MFPipeImpl::PipeMessagePut(
		/*[in]*/ const std::string &strChannel,
		/*[in]*/ const std::string &strEventName,
		/*[in]*/ const std::string &strEventParam,
		/*[in]*/ int _nMaxWaitMs)
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

		Message mes = { strEventName, strEventParam };
		writeDataBuffer->messages.push_back({ strChannel, std::make_shared<Message>(mes) });
		writeDataBuffer->mutex.unlock();
		return MF_HRESULT::RES_OK;
	} while (std::chrono::steady_clock::now() < end);

	std::cerr << "Timeout on adding message to write queue" << std::endl;
	return MF_HRESULT::RES_FALSE;
}

MF_HRESULT MFPipeImpl::PipeMessageGet(
		/*[in]*/ const std::string &strChannel,
		/*[out]*/ std::string *pStrEventName,
		/*[out]*/ std::string *pStrEventParam,
		/*[in]*/ int _nMaxWaitMs)
{
	auto start = std::chrono::steady_clock::now();
	auto end = start + std::chrono::milliseconds(_nMaxWaitMs);

	do
	{
		if (!readDataBuffer->mutex.try_lock_until(end))
			break;

		auto it = readDataBuffer->messages.begin();
		for (; it != readDataBuffer->messages.end(); ++it)
		{
			if (it->first == strChannel)
				break;
		}

		if (it == readDataBuffer->messages.end())
		{
			readDataBuffer->mutex.unlock();
			std::this_thread::yield();
			continue;
		}

		auto mes = it->second;
		readDataBuffer->messages.erase(it);

		*pStrEventName = mes->name;
		*pStrEventParam = mes->param;

		readDataBuffer->mutex.unlock();
		return MF_HRESULT::RES_OK;
	} while (std::chrono::steady_clock::now() < end);

	std::cerr << "Timeout on getting message from read queue" << std::endl;
	return MF_HRESULT::RES_FALSE;
}

MF_HRESULT MFPipeImpl::PipeFlush( /*[in]*/ const std::string &strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags)
{
	return MF_HRESULT::NOTIMPL;
}

MF_HRESULT MFPipeImpl::PipeClose()
{
	if (reader)
		reader->stop();
	if (writer)
		writer->stop();

	return io->close() ? MF_HRESULT::RES_OK : MF_HRESULT::RES_FALSE;
}
