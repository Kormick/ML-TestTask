#ifndef UDP_HPP
#define UDP_HPP

#define PACKETS_COUNT	(8)

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr)	static_cast<size_t>(sizeof(arr)/sizeof((arr)[0]))
#endif // SIZEOF_ARRAY

#include <future>
#include "MFPipeImpl.h"
#include "MFTypes.h"

static constexpr auto udpAddr = "udp://127.0.0.10:49152";
static constexpr auto udpAddr1 = "udp://127.0.0.11:49152";

bool testUdpBuffer(bool read)
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
//		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		size_t cbSize = 32 * 1024;
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
		if (writePipe.PipeCreate(udpAddr, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(udpAddr, 32, "W") == MF_HRESULT::RES_OK)
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
		if (readPipe.PipeOpen(udpAddr, 32, "R") == MF_HRESULT::RES_OK)
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

bool testUdpFrame(bool read)
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
		if (writePipe.PipeCreate(udpAddr, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(udpAddr, 32, "W") == MF_HRESULT::RES_OK)
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
		if (readPipe.PipeOpen(udpAddr, 32, "R") == MF_HRESULT::RES_OK)
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

bool testUdpMessage(bool read)
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
		if (writePipe.PipeCreate(udpAddr, "") == MF_HRESULT::RES_OK)
		{
			if (writePipe.PipeOpen(udpAddr, 32, "W") == MF_HRESULT::RES_OK)
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
		readPipe.PipeOpen(udpAddr, 0, "");

		if (readPipe.PipeOpen(udpAddr, 32, "R") == MF_HRESULT::RES_OK)
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

bool testUdpAll(bool read)
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
		if (MFPipe_Write.PipeCreate(udpAddr, "") == MF_HRESULT::RES_OK)
		{
			if (MFPipe_Write.PipeOpen(udpAddr, 32, "W") == MF_HRESULT::RES_OK)
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
		if (MFPipe_Read.PipeOpen(udpAddr, 32, "R") == MF_HRESULT::RES_OK)
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
				//		MFPipe_Read.PipeMessageGet("ch2", NULL, &arrStrings[2], 100);

				MFPipe_Read.PipeGet("ch1", arrBuffersOut[2], 1000, "");
				MFPipe_Read.PipeGet("ch2", arrBuffersOut[3], 1000, "");
				//		MFPipe_Read.PipeGet("ch2", arrBuffersOut[6], 100, "");

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

bool testUdpBufferMultithreadedWrite(std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	std::cout << "WRITE" << std::endl;

	bool testRes = true;

	MFPipeImpl writePipe;
	if (writePipe.PipeCreate(udpAddr, "") == MF_HRESULT::RES_OK)
	{
		if (writePipe.PipeOpen(udpAddr, 32, "W") == MF_HRESULT::RES_OK)
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

bool testUdpBufferMultithreadedRead(std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	std::cout << "READ" << std::endl;

	bool testRes = true;

	MFPipeImpl readPipe;
	if (readPipe.PipeOpen(udpAddr, 32, "R") == MF_HRESULT::RES_OK)
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

bool testUdpBufferMultithreaded()
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

	auto writeFut = std::async(&testUdpBufferMultithreadedWrite, arrBuffersIn, SIZEOF_ARRAY(arrBuffersIn));
	auto readFut = std::async(&testUdpBufferMultithreadedRead, arrBuffersIn, SIZEOF_ARRAY(arrBuffersIn));

	return writeFut.get() && readFut.get();
}

bool testUdp(bool read)
{
	bool res = true;

	res = res && testUdpBuffer(read);
	res = res && testUdpFrame(read);
	res = res && testUdpMessage(read);
	res = res && testUdpAll(read);

	return res;
}

bool testUdpMultithreaded()
{
	bool res = true;

	res = res && testUdpBufferMultithreaded();

	return res;
}

#endif // UDP_HPP
