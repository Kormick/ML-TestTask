#ifndef PIPEPROCESS_HPP
#define PIPEPROCESS_HPP

#include <chrono>
#include <future>

#include "../MFPipeImpl.h"
#include "../MFTypes.h"

#define PACKETS_COUNT	(8)

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr)	static_cast<size_t>(sizeof(arr)/sizeof((arr)[0]))
#endif // SIZEOF_ARRAY

#ifdef unix
static constexpr auto testPipeName = "./testPipe";
#else
static constexpr auto testPipeName = "\\\\.\\pipe\\testPipe";
#endif

/**
 * @brief Tests read/write of MF_BUFFER through pipe.
 *        Should be run for reader and writer simultaneously.
 * @param read If true tests reader process, othrewise writer process.
 * @return true if successful.
 */
bool testBuffer(const std::string &pipeName, bool read)
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();
		arrBuffersIn[i]->flags = eMFBF_Buffer;
		for (size_t j = 0; j < cbSize; ++j) {
			arrBuffersIn[i]->data.push_back(j);
		}
	}

	if (!read)
	{
		MFPipeImpl writePipe;

		auto res = writePipe.PipeCreate(pipeName, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to create pipe for write: " << res << std::endl;
			return false;
		}

		res = writePipe.PipeOpen(pipeName, 32, "W");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on write: " << res << std::endl;
			return false;
		}

		for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
		{
			res = writePipe.PipePut("", arrBuffersIn[i], 1000, "");
			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Pipe write failed: " << res << std::endl;
				return false;
			}
		}

		res = writePipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
	else
	{
		MFPipeImpl readPipe;

		auto res = readPipe.PipeOpen(pipeName, 32, "R");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on read: " << res << std::endl;
			return false;
		}

		for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
		{
			std::shared_ptr<MF_BASE_TYPE> buf;
			auto res = readPipe.PipeGet("", buf, 1000, "");

			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Read " << i << " failed: " << res << std::endl;
				return false;
			}

			if (buf == nullptr)
			{
				std::cerr << "Read " << i << " failed: data is nullptr" << std::endl;
				return false;
			}

			const auto bp = dynamic_cast<MF_BUFFER *>(buf.get());
			if (bp == nullptr)
			{
				std::cerr << "Read " << i << " failed: invalid data type" << std::endl;
				return false;
			}

			const auto dp = dynamic_cast<MF_BUFFER *>(arrBuffersIn[i].get());

			if (*dp != *bp)
			{
				std::cerr << "Read " << i << " failed: invalid data" << std::endl;
				return false;
			}
		}

		res = readPipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
}

/**
 * @brief Tests read/write of MF_FRAME through pipe.
 *        Should be run for reader and writer simultaneously.
 * @param read If true tests reader process, othrewise writer process.
 * @return true if successful.
 */
bool testFrame(const std::string &pipeName, bool read)
{
	std::shared_ptr<MF_FRAME> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		arrBuffersIn[i] = std::make_shared<MF_FRAME>();

		arrBuffersIn[i]->time.rtStartTime = i;
		arrBuffersIn[i]->time.rtEndTime = i + 1;

		arrBuffersIn[i]->av_props.vidProps.fccType = eMFCC_I420;
		arrBuffersIn[i]->av_props.vidProps.nWidth = rand() % 2048;
		arrBuffersIn[i]->av_props.vidProps.nHeight = rand() % 2048;
		arrBuffersIn[i]->av_props.vidProps.nRowBytes = rand() % 512;
		arrBuffersIn[i]->av_props.vidProps.nAspectX = rand() % 512;
		arrBuffersIn[i]->av_props.vidProps.nAspectY = rand() % 512;
		arrBuffersIn[i]->av_props.vidProps.dblRate = 12.34;

		arrBuffersIn[i]->av_props.audProps.nChannels = rand() % 8;
		arrBuffersIn[i]->av_props.audProps.nSamplesPerSec = rand() % 512;
		arrBuffersIn[i]->av_props.audProps.nBitsPerSample = rand() % 512;
		arrBuffersIn[i]->av_props.audProps.nTrackSplitBits = rand() % 512;

		arrBuffersIn[i]->str_user_props = "some_props" + std::to_string(i);

		size_t vidSize = 128 * 1024 + rand() % (256 * 1024);
		for (size_t j = 0; j < vidSize; ++j)
			arrBuffersIn[i]->vec_video_data.push_back(j);

		size_t audSize = 128 * 1024 + rand() % (256 * 1024);
		for (size_t j = 0; j < audSize; ++j)
			arrBuffersIn[i]->vec_audio_data.push_back(j);
	}

	if (!read)
	{
		MFPipeImpl writePipe;

		auto res = writePipe.PipeCreate(pipeName, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to create pipe for write: " << res << std::endl;
			return false;
		}

		res = writePipe.PipeOpen(pipeName, 32, "W");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Pipe write failed: " << res << std::endl;
			return false;
		}

		for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
		{
			res = writePipe.PipePut("", arrBuffersIn[i], 1000, "");
			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Write " << i << " failed: " << res << std::endl;
				return false;
			}
		}

		res = writePipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
	else
	{
		MFPipeImpl readPipe;

		auto res = readPipe.PipeOpen(pipeName, 32, "R");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on read: " << res << std::endl;
			return false;
		}

		for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
		{
			std::shared_ptr<MF_BASE_TYPE> buf;
			auto res = readPipe.PipeGet("", buf, 1000, "");

			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Read " << i << " failed: " << res << std::endl;
				return false;
			}

			if (buf == nullptr)
			{
				std::cerr << "Read " << i << " failed: data is nullptr" << std::endl;
				return false;
			}

			const auto bp = dynamic_cast<MF_FRAME *>(buf.get());
			if (bp == nullptr)
			{
				std::cerr << "Read " << i << " failed: invalid data type" << std::endl;
				return false;
			}

			const auto dp = dynamic_cast<MF_FRAME *>(arrBuffersIn[i].get());

			if (*dp != *bp)
			{
				std::cerr << "Read " << i << " failed: invalid data" << std::endl;
				return false;
			}
		}

		res = readPipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
}

/**
 * @brief Tests read/write of messages through pipe.
 *        Should be run for reader and writer simultaneously.
 * @param read If true tests reader process, othrewise writer process.
 * @return true if successful.
 */
bool testMessage(const std::string &pipeName, bool read)
{
	std::string pstrEvents[] = { "event1", "event2", "event3", "event4",
								 "event5", "event6", "event7", "event8" };
	std::string pstrMessages[] = { "message1", "message2", "message3", "message4",
								   "message5", "message6", "message7", "message8" };

	if (!read)
	{
		MFPipeImpl writePipe;

		auto res = writePipe.PipeCreate(pipeName, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to create pipe: " << std::endl;
			return false;
		}

		res = writePipe.PipeOpen(pipeName, 32, "W");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on write: " << res << std::endl;
			return false;
		}

		for (size_t i = 0; i < SIZEOF_ARRAY(pstrEvents); ++i)
		{
			auto res = writePipe.PipeMessagePut("", pstrEvents[i], pstrMessages[i], 1000);

			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Pipe write failed: " << res << std::endl;
				break;
			}
		}

		res = writePipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
	else
	{
		MFPipeImpl readPipe;

		auto res = readPipe.PipeOpen(pipeName, 32, "R");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on read: " << res << std::endl;
			return false;
		}

		for (size_t i = 0; i < SIZEOF_ARRAY(pstrEvents); ++i)
		{
			std::string name;
			std::string param;

			auto res = readPipe.PipeMessageGet("", &name, &param, 1000);
			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Pipe read failed: " << res << std::endl;
				return false;
			}

			if (name != pstrEvents[i] || param != pstrMessages[i])
			{
				std::cout << "Read invalid data." << std::endl;
				std::cout << "Expected: " << pstrEvents[i] << " " << pstrMessages[i] << std::endl;
				std::cout << "Got: " << name << " " << param << std::endl;
				return false;
			}
		}

		res = readPipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
}

/**
 * @brief Tests read/write of buffers and messages through pipe.
 *        Should be run for reader and writer simultaneously.
 * @param read If true tests reader process, othrewise writer process.
 * @return true if successful.
 */
bool testAll(const std::string &pipeName, bool read)
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

		arrBuffersIn[i]->flags = eMFBF_Buffer;
		for (size_t j = 0; j < cbSize; ++j) {
			arrBuffersIn[i]->data.push_back(j);
		}
	}

	std::string pstrEvents[] = { "event1", "event2", "event3", "event4",
								 "event5", "event6", "event7", "event8" };
	std::string pstrMessages[] = { "message1", "message2", "message3", "message4",
								   "message5", "message6", "message7", "message8" };

	if (!read)
	{
		MFPipeImpl writePipe;

		auto res = writePipe.PipeCreate(pipeName, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to create pipe: " << res << std::endl;
			return false;
		}

		res = writePipe.PipeOpen(pipeName, 32, "W");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on write: " << res << std::endl;
			return false;
		}

		auto tryPut = [&](const std::string &ch, std::shared_ptr<MF_BUFFER> &buf) -> bool {
			auto res = writePipe.PipePut(ch, buf, 1000, "");
			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Pipe write failed: " << res << std::endl;
				return false;
			}
			return true;
		};

		auto tryMessagePut = [&](const std::string &ch, const std::string &event, const std::string &mes) -> bool {
			auto res = writePipe.PipeMessagePut(ch, event, mes, 1000);
			if (res != MF_HRESULT::RES_OK)
			{
				std::cerr << "Pipe message write failed: " << res <<std::endl;
				return false;
			}
			return true;
		};

		for (int i = 0; i < 8; ++i)
		{
			bool writeRes = true;

			writeRes = writeRes && tryPut("ch1", arrBuffersIn[i % PACKETS_COUNT]);
			writeRes = writeRes && tryPut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT]);
			writeRes = writeRes && tryMessagePut("ch1", pstrEvents[i % PACKETS_COUNT], pstrMessages[i % PACKETS_COUNT]);
			writeRes = writeRes && tryMessagePut("ch2", pstrEvents[(i + 1) % PACKETS_COUNT], pstrMessages[(i + 1) % PACKETS_COUNT]);
			writeRes = writeRes && tryPut("ch1", arrBuffersIn[i % PACKETS_COUNT]);
			writeRes = writeRes && tryPut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT]);

			if (!writeRes)
			{
				std::cerr << "Pipe write " << i << " failed" << std::endl;
				return false;
			}
		}

		res = writePipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
	else
	{
		MFPipeImpl readPipe;

		auto res = readPipe.PipeOpen(pipeName, 32, "R");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to open pipe on read: " << res << std::endl;
			return false;
		}

		for (int i = 0; i < 8; ++i)
		{
			std::string arrStrings[4];

			std::shared_ptr<MF_BASE_TYPE> arrBuffersOut[8];
			readPipe.PipeGet("ch1", arrBuffersOut[0], 1000, "");
			readPipe.PipeGet("ch2", arrBuffersOut[1], 1000, "");

			readPipe.PipeMessageGet("ch1", &arrStrings[0], &arrStrings[1], 1000);
			readPipe.PipeMessageGet("ch2", &arrStrings[2], &arrStrings[3], 1000);

			readPipe.PipeGet("ch1", arrBuffersOut[2], 1000, "");
			readPipe.PipeGet("ch2", arrBuffersOut[3], 1000, "");

			bool readRes = true;
			if (pstrEvents[i % PACKETS_COUNT] != arrStrings[0]
					|| pstrMessages[i % PACKETS_COUNT] != arrStrings[1]
					|| pstrEvents[(i + 1) % PACKETS_COUNT] != arrStrings[2]
					|| pstrMessages[(i + 1) % PACKETS_COUNT] != arrStrings[3])
			{
				std::cerr << "Read " << i << ": " << "Invalid messages" << std::endl;
				readRes = false;
			}

			if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[i % PACKETS_COUNT].get())
					!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[0].get()))
			{
				std::cerr << "Read " << i << ": " << "Buffer 0 invalid" << std::endl;
				readRes = false;
			}

			if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[(i + 1) % PACKETS_COUNT].get())
					!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[1].get()))
			{
				std::cerr << "Read " << i << ": " << "Buffer 1 invalid" << std::endl;
				readRes = false;
			}

			if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[i % PACKETS_COUNT].get())
					!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[2].get()))
			{
				std::cerr << "Read " << i << ": " << "Buffer 2 invalid" << std::endl;
				readRes = false;
			}

			if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[(i + 1) % PACKETS_COUNT].get())
					!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[3].get()))
			{
				std::cerr << "Read " << i << ": " << "Buffer 3 invalid" << std::endl;
				readRes = false;
			}

			if (!readRes)
			{
				std::cerr << "Read " << i << " failed" << std::endl;
				return false;
			}
		}

		res = readPipe.PipeClose();
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Failed to close pipe: " << res << std::endl;
			return false;
		}

		return true;
	}
}

/**
 * @brief Test buffer write through pipe which is spawned in different thread.
 * @param buffers Pointer to buffers
 * @param size Size of the buffers
 * @return true if successful, otherwise false.
 */
bool testBufferMultithreadedOwnThreadsWrite(const std::string &pipeName, std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	MFPipeImpl writePipe;

	auto res = writePipe.PipeCreate(pipeName, "");
	if (res != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to create pipe: " << res << std::endl;
		return false;
	}

	res = writePipe.PipeOpen(pipeName, 32, "W");
	if (res != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to open pipe on write: " << res << std::endl;
		return false;
	}

	for (size_t i = 0; i < size; ++i)
	{
		auto res = writePipe.PipePut("", buffers[i], 1000, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Pipe write " << i << " failed: " << res << std::endl;
			return false;
		}
	}

	res = writePipe.PipeClose();
	if (res != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to close pipe: " << res << std::endl;
		return false;
	}

	return true;
}

/**
 * @brief Test buffer read through pipe which is spawned in different thread.
 * @param buffers Pointer to buffers
 * @param size Size of the buffers
 * @return true if successful, otherwise false.
 */
bool testBufferMultithreadedOwnThreadsRead(const std::string &pipeName, std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	MFPipeImpl readPipe;

	auto res = readPipe.PipeOpen(pipeName, 32, "R");
	if (res != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to open pipe on read: " << res << std::endl;
		return false;
	}

	for (size_t i = 0; i < size; ++i)
	{
		std::shared_ptr<MF_BASE_TYPE> buf;
		auto res = readPipe.PipeGet("", buf, 1000, "");

		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Pipe read " << i << " failed: " << res << std::endl;
			return false;
		}

		if (buf == nullptr)
		{
			std::cerr << "Pipe read " << i << ": data is nullptr" << std::endl;
			return false;
		}

		const auto dp = dynamic_cast<MF_BUFFER *>(buffers[i].get());
		const auto bp = dynamic_cast<MF_BUFFER *>(buf.get());

		if (*dp != *bp)
		{
			std::cerr << "Pipe read " << i << ": read invalid data" << std::endl;
			return false;
		}
	}

	res = readPipe.PipeClose();
	if (res != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to close pipe: " << res << std::endl;
		return false;
	}

	return true;
}

/**
 * @brief Tests read/write buffers through pipe. Each pipe is spawned in own thread.
 * @return true if successful, otherwise false.
 */
bool testBufferMultithreadedOwnThreads(const std::string &pipeName)
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

		arrBuffersIn[i]->flags = eMFBF_Buffer;
		for (size_t j = 0; j < cbSize; ++j) {
			arrBuffersIn[i]->data.push_back(j);
		}
	}

	auto writeFut = std::async(&testBufferMultithreadedOwnThreadsWrite,
							   pipeName, arrBuffersIn, SIZEOF_ARRAY(arrBuffersIn));
	auto readFut = std::async(&testBufferMultithreadedOwnThreadsRead,
							  pipeName, arrBuffersIn, SIZEOF_ARRAY(arrBuffersIn));

	return writeFut.get() && readFut.get();
}

/**
 * @brief Tests write buffers through pipe with multiple write threads.
 * @param pipe Pointer to pipe
 * @param buffer Buffer to write
 * @return true is successful, otherwise false.
 */
bool testBufferMultithreadedWrite(MFPipeImpl *pipe, std::shared_ptr<MF_BUFFER> buffer)
{
	for (auto i = 0; i < 128; ++i)
	{
		auto res = pipe->PipePut("", buffer, 1000, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Write " << i << " failed: " << res << std::endl;
			return false;
		}
	}

	return true;
}

/**
 * @brief Test read buffers through pipe with multiple read threads.
 * @param pipe Pointer to pipe
 * @param buffer Buffer to read
 * @return true if successful, otherwise false.
 */
bool testBufferMultithreadedRead(MFPipeImpl *pipe, std::shared_ptr<MF_BUFFER> buffer)
{
	for (auto i = 0; i < 128; ++i)
	{
		std::shared_ptr<MF_BASE_TYPE> out;

		auto res = pipe->PipeGet("", out, 1000, "");
		if (res != MF_HRESULT::RES_OK)
		{
			std::cerr << "Read " << i << " failed: " << res << std::endl;
			return false;
		}

		if (out == nullptr)
		{
			std::cerr << "Read " << i << " failed: data is nullptr" << std::endl;
			return false;
		}

		const auto dp = dynamic_cast<MF_BUFFER *>(buffer.get());
		const auto bp = dynamic_cast<MF_BUFFER *>(out.get());

		if (*dp != *bp)
		{
			std::cerr << "Read " << i << " failed: invalid data" << std::endl;
			return false;
		}
	}

	return true;
}

bool testBufferMultithreadedOpenWrite(const std::string &pipeName, MFPipeImpl *pipe)
{
	if (pipe->PipeCreate(pipeName, "") != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to create write pipe" << std::endl;
		return false;
	}
	if (pipe->PipeOpen(pipeName, 32, "W") != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to open write pipe" << std::endl;
		return false;
	}

	return true;
}

bool testBufferMultithreadedOpenRead(const std::string &pipeName, MFPipeImpl *pipe)
{
	if (pipe->PipeOpen(pipeName, 32, "R") != MF_HRESULT::RES_OK)
	{
		std::cerr << "Failed to open read pipe" << std::endl;
		return false;
	}

	return true;
}

bool testBufferMultithreaded(const std::string &pipeName)
{
	std::shared_ptr<MF_BUFFER> buffer = std::make_shared<MF_BUFFER>();
	buffer->flags = eMFBF_Buffer;
	for (auto i = 0; i < 64 * 1024; ++i)
		buffer->data.push_back(i);

	MFPipeImpl writePipe;
	MFPipeImpl readPipe;

	auto writeOpenFut = std::async(&testBufferMultithreadedOpenWrite, pipeName, &writePipe);
	auto readOpenFut = std::async(&testBufferMultithreadedOpenRead, pipeName, &readPipe);

	if (!writeOpenFut.get() || !readOpenFut.get())
		return false;

	auto writeFut0 = std::async(&testBufferMultithreadedWrite, &writePipe, buffer);
	auto writeFut1 = std::async(&testBufferMultithreadedWrite, &writePipe, buffer);
	auto readFut0 = std::async(&testBufferMultithreadedRead, &readPipe, buffer);
	auto readFut1 = std::async(&testBufferMultithreadedRead, &readPipe, buffer);

	return writeFut0.get() && writeFut1.get() && readFut0.get() && readFut1.get();
}

bool testPipeProcess(bool read)
{
	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	bool res = true;

	{
		bool inRes = testBuffer(testPipeName, read);
		std::cout << "\ttestBuffer(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	{
		bool inRes = testFrame(testPipeName, read);
		std::cout << "\ttestFrame(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	{
		bool inRes = testMessage(testPipeName, read);
		std::cout << "\ttestMessage(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	{
		bool inRes = testAll(testPipeName, read);
		std::cout << "\ttestAll(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	return res;
}

bool testPipeMultithreaded()
{
	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	bool res = true;

	{
		bool inRes = testBufferMultithreadedOwnThreads(testPipeName);
		std::cout <<"\ttestBufferMultithreadedOwnThreads(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testBufferMultithreaded(testPipeName);
		std::cout << "\ttestBufferMultithreaded(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	return res;
}

#endif // PIPEPROCESS_HPP
