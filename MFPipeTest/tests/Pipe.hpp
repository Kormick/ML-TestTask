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
static constexpr auto pipeName = "./testPipe";
#else
static constexpr auto pipeName = "\\\\.\\pipe\\testPipe";
#endif

bool testBuffer(bool read)
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

	bool testRes = true;

	if (!read)
	{
		std::cout << "WRITE" << std::endl;

		MFPipeImpl writePipe;
		if (writePipe.PipeCreate(pipeName, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(pipeName, 32, "W") == MF_HRESULT::RES_OK)
			{
				for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
				{
					auto res = writePipe.PipePut("", arrBuffersIn[i], 10000, "");
					std::cout << "Write " << i << " res " << res << std::endl;

					if (res != MF_HRESULT::RES_OK)
					{
						std::cout << "Failed to write into pipe" << std::endl;
						testRes = false;
						break;
					}
				}
				writePipe.PipeClose();
			}
			else
			{
				std::cout << "Failed to open pipe on write." << std::endl;
				testRes = false;
			}
		}
		else
		{
			std::cout << "Failed to create pipe." << std::endl;
			testRes = false;
		}
	} else {
		std::cout << "READ" << std::endl;

		MFPipeImpl readPipe;
		if (readPipe.PipeOpen(pipeName, 32, "R") == MF_HRESULT::RES_OK)
		{
			for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
			{
				std::shared_ptr<MF_BASE_TYPE> buf;
				auto res = readPipe.PipeGet("", buf, 10000, "");
				std::cout << "Read " << i << " res " << res << std::endl;

				const auto dp = dynamic_cast<MF_BUFFER *>(arrBuffersIn[i].get());
				const auto bp = dynamic_cast<MF_BUFFER *>(buf.get());

				if (*dp != *bp)
				{
					std::cout << "Read invalid data." << std::endl;
					testRes = false;
				}
			}

			readPipe.PipeClose();
		}
		else
		{
			std::cout << "Failed to open pipe on read" << std::endl;
			testRes = false;
		}
	}

	return testRes;
}

bool testFrame(bool read)
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

	bool testRes = true;

	if (!read)
	{
		std::cout << "WRITE" << std::endl;

		MFPipeImpl writePipe;
		if (writePipe.PipeCreate(pipeName, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(pipeName, 32, "W") == MF_HRESULT::RES_OK)
			{
				for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
				{
					auto res = writePipe.PipePut("", arrBuffersIn[i], 1000, "");
					std::cout << "Write " << i << " res " << res << std::endl;

					if (res != MF_HRESULT::RES_OK)
					{
						std::cout << "Failed to write into pipe" << std::endl;
						testRes = false;
						break;
					}
				}
			}
			else
			{
				std::cout << "Failed to open pipe on write." << std::endl;
				testRes = false;
			}
		}
		else
		{
			std::cout << "Faailed to create pipe." << std::endl;
			testRes = false;
		}

		writePipe.PipeClose();
	} else {
		std::cout << "READ" << std::endl;

		MFPipeImpl readPipe;
		if (readPipe.PipeOpen(pipeName, 32, "R") == MF_HRESULT::RES_OK)
		{
			for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
			{
				std::shared_ptr<MF_BASE_TYPE> buf;
				auto res = readPipe.PipeGet("", buf, 1000, "");
				std::cout << "Read " << i << " res " << res << std::endl;

				const auto dp = dynamic_cast<MF_FRAME *>(arrBuffersIn[i].get());
				const auto bp = dynamic_cast<MF_FRAME *>(buf.get());

				if (*dp != *bp)
				{
					std::cout << "Read invalid data." << std::endl;
					testRes = false;
				}
			}
		}
		else
		{
			std::cout << "Failed to open pipe on read" << std::endl;
			testRes = false;
		}

		readPipe.PipeClose();
	}

	return testRes;
}

bool testMessage(bool read)
{
	std::string pstrEvents[] = { "event1", "event2", "event3", "event4",
								 "event5", "event6", "event7", "event8" };
	std::string pstrMessages[] = { "message1", "message2", "message3", "message4",
								   "message5", "message6", "message7", "message8" };

	bool testRes = true;

	if (!read)
	{
		std::cout << "WRITE" << std::endl;

		MFPipeImpl writePipe;
		if (writePipe.PipeCreate(pipeName, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(pipeName, 32, "W") == MF_HRESULT::RES_OK)
			{
				for (size_t i = 0; i < SIZEOF_ARRAY(pstrEvents); ++i)
				{
					auto res = writePipe.PipeMessagePut("", pstrEvents[i], pstrMessages[i], 1000);
					std::cout << "Message " << i << " write res " << res << std::endl;

					if (res != MF_HRESULT::RES_OK)
					{
						std::cout << "Failed to write message into pipe." << std::endl;
						testRes = false;
						break;
					}
				}

				writePipe.PipeClose();
			}
			else
			{
				std::cout << "Failed to open pipe on write." << std::endl;
				testRes = false;
			}
		}
		else
		{
			std::cout << "Failed to create pipe." << std::endl;
			testRes = false;
		}
	}
	else
	{
		std::cout << "READ" << std::endl;

		MFPipeImpl readPipe;
		readPipe.PipeOpen(pipeName, 0, "");

		if (readPipe.PipeOpen(pipeName, 32, "R") == MF_HRESULT::RES_OK)
		{
			for (size_t i = 0; i < SIZEOF_ARRAY(pstrEvents); ++i)
			{
				std::string name;
				std::string param;

				auto res = readPipe.PipeMessageGet("", &name, &param, 1000);
				std::cout << "Message " << i << " read res " << res << std::endl;

				if (name != pstrEvents[i] || param != pstrMessages[i])
				{
					std::cout << "Read invalid data." << std::endl;
					std::cout << "Expected: " << pstrEvents[i] << " " << pstrMessages[i] << std::endl;
					std::cout << "Got: " << name << " " << param << std::endl;
					testRes = false;
				}
			}

			readPipe.PipeClose();
		}
		else
		{
			std::cout << "Failed to open pipe on read" << std::endl;
			testRes = false;
		}
	}

	return testRes;
}

bool testAll(bool read)
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

		arrBuffersIn[i]->flags = eMFBF_Buffer;
		// Fill buffer
		// TODO: fill by test data here

		for (size_t j = 0; j < cbSize; ++j) {
			arrBuffersIn[i]->data.push_back(j);
		}
	}

	std::string pstrEvents[] = { "event1", "event2", "event3", "event4",
								 "event5", "event6", "event7", "event8" };
	std::string pstrMessages[] = { "message1", "message2", "message3", "message4",
								   "message5", "message6", "message7", "message8" };

	bool testRes = true;

	if (!read)
	{
		// Write pipe
		MFPipeImpl MFPipe_Write;
		if (MFPipe_Write.PipeCreate(pipeName, "") == MF_HRESULT::RES_OK)
		{
			if (MFPipe_Write.PipeOpen(pipeName, 32, "W") == MF_HRESULT::RES_OK)
			{
				for (int i = 0; i < 8; ++i)
				{
					std::cout << "Write " << i << std::endl;

					// Write to pipe
					MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 1000, "");
					MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 1000, "");
					MFPipe_Write.PipeMessagePut("ch1", pstrEvents[i % PACKETS_COUNT], pstrMessages[i % PACKETS_COUNT], 1000);
					MFPipe_Write.PipeMessagePut("ch2", pstrEvents[(i + 1) % PACKETS_COUNT], pstrMessages[(i + 1) % PACKETS_COUNT], 1000);

					MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 1000, "");
					MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 1000, "");

					std::string strPipeName;
					MFPipe_Write.PipeInfoGet(&strPipeName, "", NULL);

					MFPipe::MF_PIPE_INFO mfInfo = {};
					MFPipe_Write.PipeInfoGet(NULL, "ch2", &mfInfo);
					MFPipe_Write.PipeInfoGet(NULL, "ch1", &mfInfo);
				}
			}
			else
			{
				std::cout << "Failed to open pipe on write." << std::endl;
				testRes = false;
			}
		}
		else
		{
			std::cout << "Failed to create write pipe" << std::endl;
			testRes = false;
		}
	}
	else
	{
		// Read pipe
		MFPipeImpl MFPipe_Read;
		if (MFPipe_Read.PipeOpen(pipeName, 32, "R") == MF_HRESULT::RES_OK)
		{
			for (int i = 0; i < 8; ++i)
			{
				std::cout << "Read " << i << std::endl;

				// Read from pipe
				std::string arrStrings[4];

				std::shared_ptr<MF_BASE_TYPE> arrBuffersOut[8];
				MFPipe_Read.PipeGet("ch1", arrBuffersOut[0], 1000, "");
				MFPipe_Read.PipeGet("ch2", arrBuffersOut[1], 1000, "");

				MFPipe_Read.PipeMessageGet("ch1", &arrStrings[0], &arrStrings[1], 1000);
				MFPipe_Read.PipeMessageGet("ch2", &arrStrings[2], &arrStrings[3], 1000);

				MFPipe_Read.PipeGet("ch1", arrBuffersOut[2], 1000, "");
				MFPipe_Read.PipeGet("ch2", arrBuffersOut[3], 1000, "");

				// TODO: Your test code here

				if (pstrEvents[i % PACKETS_COUNT] != arrStrings[0]
						|| pstrMessages[i % PACKETS_COUNT] != arrStrings[1]
						|| pstrEvents[(i + 1) % PACKETS_COUNT] != arrStrings[2]
						|| pstrMessages[(i + 1) % PACKETS_COUNT] != arrStrings[3])
				{
					std::cout << "Message failed" << std::endl;
					testRes = false;
				}

				if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[i % PACKETS_COUNT].get())
						!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[0].get()))
				{
					std::cout << "Buffer 0 failed" << std::endl;
					testRes = false;
				}

				if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[(i + 1) % PACKETS_COUNT].get())
						!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[1].get()))
				{
					std::cout << "Buffer 1 failed" << std::endl;
					testRes = false;
				}

				if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[i % PACKETS_COUNT].get())
						!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[2].get()))
				{
					std::cout << "Buffer 2 failed" << std::endl;
					testRes = false;
				}

				if (*dynamic_cast<MF_BUFFER *>(arrBuffersIn[(i + 1) % PACKETS_COUNT].get())
						!= *dynamic_cast<MF_BUFFER *>(arrBuffersOut[3].get()))
				{
					std::cout << "Buffer 3 failed" << std::endl;
					testRes = false;
				}
			}
		}
		else
		{
			std::cout << "Failed to open pipe on read" << std::endl;
			testRes = false;
		}
	}

	return testRes;
}

bool testBufferMultithreadedWrite(std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	std::cout << "WRITE" << std::endl;

	bool testRes = true;

	MFPipeImpl writePipe;
	if (writePipe.PipeCreate(pipeName, "") == MF_HRESULT::RES_OK)
	{
		if (writePipe.PipeOpen(pipeName, 32, "W") == MF_HRESULT::RES_OK)
		{
			for (size_t i = 0; i < size; ++i)
			{
				auto res = writePipe.PipePut("", buffers[i], 1000, "");
				std::cout << "Write " << i << " res " << res << std::endl;

				if (res != MF_HRESULT::RES_OK)
				{
					std::cout << "Failed to write into pipe" << std::endl;
					testRes = false;
					break;
				}
			}
		}
		else
		{
			std::cout << "Failed to open pipe on write." << std::endl;
			testRes = false;
		}

	}
	else
	{
		std::cout << "Failed to create pipe." << std::endl;
		testRes = false;
	}

	writePipe.PipeClose();

	return testRes;
}

bool testBufferMultithreadedRead(std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	std::cout << "READ" << std::endl;

	bool testRes = true;

	MFPipeImpl readPipe;
	if (readPipe.PipeOpen(pipeName, 32, "R") == MF_HRESULT::RES_OK)
	{
		for (size_t i = 0; i < size; ++i)
		{
			std::shared_ptr<MF_BASE_TYPE> buf;
			auto res = readPipe.PipeGet("", buf, 1000, "");
			std::cout << "Read " << i << " res " << res << std::endl;

			const auto dp = dynamic_cast<MF_BUFFER *>(buffers[i].get());
			const auto bp = dynamic_cast<MF_BUFFER *>(buf.get());

			if (*dp != *bp)
			{
				std::cout << "Read invalid data." << std::endl;
				testRes = false;
			}
		}
	}
	else
	{
		std::cout << "Failed to open pipe on read." << std::endl;
		testRes = false;
	}

	readPipe.PipeClose();

	return testRes;
}

bool testBufferMultithreaded()
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

		arrBuffersIn[i]->flags = eMFBF_Buffer;
		// Fill buffer
		// TODO: fill by test data here

		for (size_t j = 0; j < cbSize; ++j) {
			arrBuffersIn[i]->data.push_back(j);
		}
	}

	auto writeFut = std::async(&testBufferMultithreadedWrite, arrBuffersIn, SIZEOF_ARRAY(arrBuffersIn));
	auto readFut = std::async(&testBufferMultithreadedRead, arrBuffersIn, SIZEOF_ARRAY(arrBuffersIn));

	return writeFut.get() && readFut.get();
}

bool testBufferMultithreaded2Write(MFPipeImpl *pipe, std::shared_ptr<MF_BUFFER> buffer)
{
	for (auto i = 0; i < 128; ++i)
	{
		if (pipe->PipePut("", buffer, 1000, "") != MF_HRESULT::RES_OK)
		{
			std::cout << "Write failed on " << i << std::endl;
			return false;
		}

		std::cout << "Write " << i << std::endl;
	}

	return true;
}

bool testBufferMultithreaded2Read(MFPipeImpl *pipe, std::shared_ptr<MF_BUFFER> buffer)
{
	for (auto i = 0; i < 128; ++i)
	{
		std::shared_ptr<MF_BASE_TYPE> out;
		if (pipe->PipeGet("", out, 1000, "") != MF_HRESULT::RES_OK)
		{
			std::cout << "Read failed on " << i << std::endl;
			return false;
		}

		if (out == nullptr)
		{
			std::cout << "Returned empty data on " << i << std::endl;
			return false;
		}

		const auto dp = dynamic_cast<MF_BUFFER *>(buffer.get());
		const auto bp = dynamic_cast<MF_BUFFER *>(out.get());

		if (*dp != *bp)
		{
			std::cout << "Read invalid data on " << i << std::endl;
			return false;
		}
		else
		{
			std::cout << "Read " << i << std::endl;
		}
	}

	return true;
}

bool testBufferMultithreaded2OpenWrite(MFPipeImpl *pipe)
{
	if (pipe->PipeCreate(pipeName, "") != MF_HRESULT::RES_OK)
	{
		std::cout << "Failed to create write pipe" << std::endl;
		return false;
	}
	if (pipe->PipeOpen(pipeName, 32, "W") != MF_HRESULT::RES_OK)
	{
		std::cout << "Failed to open write pipe" << std::endl;
		return false;
	}

	return true;
}

bool testBufferMultithreaded2OpenRead(MFPipeImpl *pipe)
{
	if (pipe->PipeOpen(pipeName, 32, "R") != MF_HRESULT::RES_OK)
	{
		std::cout << "Failed to open read pipe" << std::endl;
		return false;
	}

	return true;
}

bool testBufferMultithreaded2()
{
	std::shared_ptr<MF_BUFFER> buffer = std::make_shared<MF_BUFFER>();
	buffer->flags = eMFBF_Buffer;
	for (auto i = 0; i < 64 * 1024; ++i)
		buffer->data.push_back(i);

	MFPipeImpl writePipe;
	MFPipeImpl readPipe;

	auto writeOpenFut = std::async(&testBufferMultithreaded2OpenWrite, &writePipe);
	auto readOpenFut = std::async(&testBufferMultithreaded2OpenRead, &readPipe);

	if (!writeOpenFut.get() || !readOpenFut.get())
		return false;

	auto writeFut0 = std::async(&testBufferMultithreaded2Write, &writePipe, buffer);
	auto writeFut1 = std::async(&testBufferMultithreaded2Write, &writePipe, buffer);
	auto readFut0 = std::async(&testBufferMultithreaded2Read, &readPipe, buffer);
	auto readFut1 = std::async(&testBufferMultithreaded2Read, &readPipe, buffer);

	return writeFut0.get() && writeFut1.get() && readFut0.get() && readFut1.get();
}

bool testPerformance(bool read)
{
	std::shared_ptr<MF_BUFFER> buffer = std::make_shared<MF_BUFFER>();
	buffer->flags = eMFBF_Buffer;
	buffer->data.reserve(1024 * 1024);
	for (auto i = 0; i < 1024 * 1024; ++i)
		buffer->data.push_back(i);

	bool testRes = true;

	if (!read)
	{
		auto start = std::chrono::steady_clock::now();

		MFPipeImpl writePipe;
		if (writePipe.PipeCreate(pipeName, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(pipeName, 32, "W") == MF_HRESULT::RES_OK)
			{
				for (auto i = 0; i < 1024 && testRes; ++i)
				{
					if (writePipe.PipePut("", buffer, 10000, "") != MF_HRESULT::RES_OK)
					{
						std::cout << "Failed to put buffer in write queue" << std::endl;
						testRes = false;
					}
				}

				writePipe.PipeClose();
			}
			else
			{
				std::cout << "Failed to open pipe on write" << std::endl;
				testRes = false;
			}
		}
		else
		{
			std::cout << "Failed to create pipe" << std::endl;
		}

		auto end = std::chrono::steady_clock::now();

		auto time = end - start;
		std::cout << "Written 1Gb in " << std::chrono::duration_cast<std::chrono::seconds>(time).count() << std::endl;
	}
	else
	{
		auto start = std::chrono::steady_clock::now();

		MFPipeImpl readPipe;
		if (readPipe.PipeOpen(pipeName, 32, "R") == MF_HRESULT::RES_OK)
		{
			for (auto i = 0; i < 1024 && testRes; ++i)
			{
				std::shared_ptr<MF_BASE_TYPE> buf;
				auto res = readPipe.PipeGet("", buf, 10000, "");

				if (res != MF_HRESULT::RES_OK)
				{
					std::cout << "Failed to get buffer from read queue" << std::endl;
					testRes = false;
				}
				else if (*dynamic_cast<MF_BUFFER *>(buf.get()) != *buffer)
				{
					std::cout << "Read invalid data" << std::endl;
					testRes = false;
				}
			}

			readPipe.PipeClose();
		}
		else
		{
			std::cout << "Failed to open pipe on read" << std::endl;
			testRes = false;
		}

		auto end = std::chrono::steady_clock::now();

		auto time = end - start;
		std::cout << "Read 1Gb in " << std::chrono::duration_cast<std::chrono::seconds>(time).count() << std::endl;
	}

	return testRes;
}

bool testPipeProcess(bool read)
{
	bool res = true;

	res = res && testBuffer(read);
	res = res && testFrame(read);
	res = res && testMessage(read);
	res = res && testAll(read);

	return res;
}

bool testPipeMultithreaded()
{
	bool res = true;

//	res = res && testBufferMultithreaded();
	res = res && testBufferMultithreaded2();

	return res;
}

bool testPipePerformance(bool read)
{
	bool res = true;

	res = res && testPerformance(read);

	return res;
}

#endif // PIPEPROCESS_HPP
