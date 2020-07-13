#ifndef MF_PIPEIMPL_H_
#define MF_PIPEIMPL_H_

#include <cstdint>
#include <deque>
#include <string>
#include <memory>

#include "IoInterface.hpp"
#include "MFPipe.h"
#include "MFTypes.h"
#include "PipeReader.hpp"
#include "PipeWriter.hpp"
class MFPipeImpl: public MFPipe
{
public:
	MFPipeImpl() = default;

	~MFPipeImpl() override;

	MF_HRESULT PipeInfoGet(
			/*[out]*/ std::string *pStrPipeName,
			/*[in]*/ const std::string &strChannel,
			MF_PIPE_INFO* _pPipeInfo) override;

	MF_HRESULT PipeCreate(
			/*[in]*/ const std::string &strPipeID,
			/*[in]*/ const std::string &strHints) override;

	MF_HRESULT PipeOpen(
			/*[in]*/ const std::string &strPipeID,
			/*[in]*/ int _nMaxBuffers,
			/*[in]*/ const std::string &strHints,
			/*[in]*/ int _nMaxWaitMs = 10000) override;

	MF_HRESULT PipePut(
			/*[in]*/ const std::string &strChannel,
			/*[in]*/ const std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override;

	MF_HRESULT PipeGet(
			/*[in]*/ const std::string &strChannel,
			/*[out]*/ std::shared_ptr<MF_BASE_TYPE> &pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override;

	MF_HRESULT PipePeek(
			/*[in]*/ const std::string &strChannel,
			/*[in]*/ int _nIndex,
			/*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
			/*[in]*/ int _nMaxWaitMs,
			/*[in]*/ const std::string &strHints) override;

	MF_HRESULT PipeMessagePut(
			/*[in]*/ const std::string &strChannel,
			/*[in]*/ const std::string &strEventName,
			/*[in]*/ const std::string &strEventParam,
			/*[in]*/ int _nMaxWaitMs) override;

	MF_HRESULT PipeMessageGet(
			/*[in]*/ const std::string &strChannel,
			/*[out]*/ std::string *pStrEventName,
			/*[out]*/ std::string *pStrEventParam,
			/*[in]*/ int _nMaxWaitMs) override;

	MF_HRESULT PipeFlush( /*[in]*/ const std::string &strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) override;

	MF_HRESULT PipeClose() override;

private:
	std::string pipeId;
	size_t maxBuffers;
	MF_PIPE_INFO pipeInfo;

	std::timed_mutex mutex;

	std::shared_ptr<IoInterface> io;

	std::shared_ptr<DataBuffer> readDataBuffer;
	std::shared_ptr<DataBuffer> writeDataBuffer;

	std::unique_ptr<PipeReader> reader;
	std::unique_ptr<PipeWriter> writer;
};

#endif
