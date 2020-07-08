#include "MFPipeImpl.h"
#include <iostream>
#include <cmath>
#include <future>

#define PACKETS_COUNT	(8)

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr)	static_cast<size_t>(sizeof(arr)/sizeof((arr)[0]))
#endif // SIZEOF_ARRAY

static constexpr auto testPipeName = "/home/kormick/workspace/testPipe";

bool testPipeCreateInvalidName()
{
	const std::string name = "";

	MFPipeImpl pipe;
	auto res = pipe.PipeCreate(name, "");
	if (res == S_OK)
	{
		std::cout << "Should not be able to crate pipe with invalid name." << std::endl;
		return false;
	}

	return true;
}

bool testPipeCreateValidName()
{
	MFPipeImpl pipe;
	auto res = pipe.PipeCreate(testPipeName, "");
	if (res != S_OK)
	{
		std::cout << "Failed to create pipe at " << testPipeName << std::endl;
		std::cout << "Result: " << res << std::endl;
		return false;
	}

	return true;
}

bool testPipeCreate()
{
	bool res = true;

	res = res && testPipeCreateInvalidName();
	res = res && testPipeCreateValidName();

	return res;
}

bool testBuffer(bool read)
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

	bool testRes = true;

	if (!read)
	{
		std::cout << "WRITE" << std::endl;

		MFPipeImpl writePipe;
		if (writePipe.PipeCreate(testPipeName, "") == S_OK)
		{
			for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
			{
				auto res = writePipe.PipePut("", arrBuffersIn[i], 10000, "");
				std::cout << "Write " << i << " res " << res << std::endl;

				if (res != S_OK)
				{
					std::cout << "Failed to write into pipe" << std::endl;
					testRes = false;
					break;
				}
			}
		}
		else
		{
			std::cout << "Failed to create pipe." << std::endl;
			testRes = false;
		}

		writePipe.PipeClose();
	} else {
		std::cout << "READ" << std::endl;

		MFPipeImpl readPipe;
		readPipe.PipeOpen(testPipeName, 0, "");

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

	return testRes;
}

bool testBufferMultithreadedWrite(std::shared_ptr<MF_BUFFER> *buffers, size_t size)
{
	std::cout << "WRITE" << std::endl;

	bool testRes = true;

	MFPipeImpl writePipe;
	if (writePipe.PipeCreate(testPipeName, "") == S_OK)
	{
		for (size_t i = 0; i < size; ++i)
		{
			auto res = writePipe.PipePut("", buffers[i], 1000, "");
			std::cout << "Write " << i << " res " << res << std::endl;

			if (res != S_OK)
			{
				std::cout << "Failed to write into pipe" << std::endl;
				testRes = false;
				break;
			}
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
	readPipe.PipeOpen(testPipeName, 0, "");

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
		if (writePipe.PipeCreate(testPipeName, "") == S_OK)
		{
			for (size_t i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
			{
				auto res = writePipe.PipePut("", arrBuffersIn[i], 1000, "");
				std::cout << "Write " << i << " res " << res << std::endl;

				if (res != S_OK)
				{
					std::cout << "Failed to write into pipe" << std::endl;
					testRes = false;
					break;
				}
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
		readPipe.PipeOpen(testPipeName, 0, "");

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
		if (writePipe.PipeCreate(testPipeName, "") == S_OK)
		{
			for (size_t i = 0; i < SIZEOF_ARRAY(pstrEvents); ++i)
			{
				auto res = writePipe.PipeMessagePut("", pstrEvents[i], pstrMessages[i], 1000);
				std::cout << "Message " << i << " write res " << res << std::endl;

				if (res != S_OK)
				{
					std::cout << "Failed to write message into pipe." << std::endl;
					testRes = false;
					break;
				}
			}
		}
		else
		{
			std::cout << "Failed to create pipe." << std::endl;
			testRes = false;
		}

		writePipe.PipeClose();
	}
	else
	{
		std::cout << "READ" << std::endl;

		MFPipeImpl readPipe;
		readPipe.PipeOpen(testPipeName, 0, "");

		for (size_t i = 0; i < SIZEOF_ARRAY(pstrEvents); ++i)
		{
			std::string name;
			std::string param;

			auto res = readPipe.PipeMessageGet("", &name, &param, 1000);
			std::cout << "Message " << i << " read res " << res << std::endl;

			if (name != pstrEvents[i] || param != pstrMessages[i])
			{
				std::cout << "Read invalid data." << std::endl;
				testRes = false;
			}
		}

		readPipe.PipeClose();
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

	std::string pstrEvents[] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
	std::string pstrMessages[] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };

	bool testRes = true;

	if (!read)
	{
		// Write pipe
		MFPipeImpl MFPipe_Write;
		MFPipe_Write.PipeCreate(testPipeName, "");

		for (int i = 0; i < 1; ++i)
		{
			std::cout << "Write " << i << std::endl;

			// Write to pipe
			{
				auto res = MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 1000, "");
				std::cout << "Write " << i << " " << res << std::endl;
			}
			{
				auto res = MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 1000, "");
				std::cout << "Write " << i << " " << res << std::endl;
			}
			{
				auto res = MFPipe_Write.PipeMessagePut("ch1", pstrEvents[i % PACKETS_COUNT], pstrMessages[i % PACKETS_COUNT], 1000);
				std::cout << "Write " << i << " " << res << std::endl;
			}
			{
				auto res = MFPipe_Write.PipeMessagePut("ch2", pstrEvents[(i + 1) % PACKETS_COUNT], pstrMessages[(i + 1) % PACKETS_COUNT], 1000);
				std::cout << "Write " << i << " " << res << std::endl;
			}

			{
				auto res = MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 1000, "");
				std::cout << "Write " << i << " " << res << std::endl;
			}
			{
				auto res = MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 1000, "");
				std::cout << "Write " << i << " " << res << std::endl;
			}

			std::string strPipeName;
			MFPipe_Write.PipeInfoGet(&strPipeName, "", NULL);

			MFPipe::MF_PIPE_INFO mfInfo = {};
			MFPipe_Write.PipeInfoGet(NULL, "ch2", &mfInfo);
			MFPipe_Write.PipeInfoGet(NULL, "ch1", &mfInfo);
		}
	}
	else
	{
		// Read pipe
		MFPipeImpl MFPipe_Read;
		MFPipe_Read.PipeOpen(testPipeName, 32, "");

		for (int i = 0; i < 1; ++i)
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

	return testRes;
}

int TestMethod1()
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (int i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

		arrBuffersIn[i]->flags = eMFBF_Buffer;
		// Fill buffer
		// TODO: fill by test data here
	}

	std::string pstrEvents[] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
	std::string pstrMessages[] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };
	

	//////////////////////////////////////////////////////////////////////////
	// Pipe creation

	// Write pipe
	MFPipeImpl MFPipe_Write;
	MFPipe_Write.PipeCreate("udp://127.0.0.1:12345", "");

	// Read pipe
	MFPipeImpl MFPipe_Read;
	MFPipe_Read.PipeOpen("udp://127.0.0.1:12345", 32, "");

	//////////////////////////////////////////////////////////////////////////
	// Test code (single-threaded)
	// TODO: multi-threaded

	// Note: channels ( "ch1", "ch2" is optional)

	for (int i = 0; i < 128; ++i)
	{
		// Write to pipe
		MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 0, "");
		MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 0, "");
		MFPipe_Write.PipeMessagePut("ch1", pstrEvents[i % PACKETS_COUNT], pstrMessages[i % PACKETS_COUNT], 100);
		MFPipe_Write.PipeMessagePut("ch2", pstrEvents[(i + 1) % PACKETS_COUNT], pstrMessages[(i + 1) % PACKETS_COUNT], 100);

		MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 0, "");
		MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 0, "");

		std::string strPipeName;
		MFPipe_Write.PipeInfoGet(&strPipeName, "", NULL);

		MFPipe::MF_PIPE_INFO mfInfo = {};
		MFPipe_Write.PipeInfoGet(NULL, "ch2", &mfInfo);
		MFPipe_Write.PipeInfoGet(NULL, "ch1", &mfInfo);

		// Read from pipe
		std::string arrStrings[4];

		MFPipe_Write.PipeMessageGet("ch1", &arrStrings[0], &arrStrings[1], 100);
		MFPipe_Write.PipeMessageGet("ch2", &arrStrings[2], &arrStrings[3], 100);
		MFPipe_Write.PipeMessageGet("ch2", NULL, &arrStrings[2], 100);

		std::shared_ptr<MF_BASE_TYPE> arrBuffersOut[8];
		MFPipe_Write.PipeGet("ch1", arrBuffersOut[0], 100, "");
		MFPipe_Write.PipeGet("ch2", arrBuffersOut[1], 100, "");

		MFPipe_Write.PipeGet("ch1", arrBuffersOut[4], 100, "");
		MFPipe_Write.PipeGet("ch2", arrBuffersOut[5], 100, "");
		MFPipe_Write.PipeGet("ch2", arrBuffersOut[6], 100, "");

		// TODO: Your test code here
	}
	
	return 0;
}

bool testParserFrame()
{
	MF_FRAME frame;

	frame.time.rtStartTime = 0;
	frame.time.rtEndTime = 1;

	frame.av_props.vidProps.fccType = eMFCC_I420;
	frame.av_props.vidProps.nWidth = 2;
	frame.av_props.vidProps.nHeight = 3;
	frame.av_props.vidProps.nRowBytes = 4;
	frame.av_props.vidProps.nAspectX = 5;
	frame.av_props.vidProps.nAspectY = 6;
	frame.av_props.vidProps.dblRate = 7.89;

	frame.av_props.audProps.nChannels = 10;
	frame.av_props.audProps.nSamplesPerSec = 11;
	frame.av_props.audProps.nBitsPerSample = 12;
	frame.av_props.audProps.nTrackSplitBits = 13;

	frame.str_user_props = "user_props";

	for (size_t i = 0; i < 10; ++i)
		frame.vec_video_data.push_back(i);
	for (size_t i = 0; i < 10; ++i)
		frame.vec_audio_data.push_back(i);

	auto bytes = frame.serialize();

	Parser parser;
	auto size = parser.parse(bytes, 0);

	if (size != bytes.size())
	{
		std::cout << "Frame parser failed: " << std::endl;
		std::cout << "Parsed " << size << ". Expected " << bytes.size() << std::endl;
		return false;
	}

	if (parser.getState() != Parser::State::FRAME_READY)
	{
		std::cout << "Frame parser failed: " << std::endl;
		std::cout << "State " << static_cast<int32_t>(parser.getState()) << std::endl;
		return false;
	}

	const auto data = parser.getData();

	bytes.erase(bytes.begin());
	if (bytes != data)
	{
		std::cout << "Frame parser failed: " << std::endl;

		{
			std::cout << "Data: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : data) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		{
			std::cout << "Expected: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : bytes) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		return false;
	}

	return true;
}

bool testParserBuffer()
{
	std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
	for (int i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i)
	{
		size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
		arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

		arrBuffersIn[i]->flags = eMFBF_Buffer;
		// Fill buffer
		// TODO: fill by test data here

		for (auto j = 0; j < i + 5; ++j) {
			arrBuffersIn[i]->data.push_back(j);
		}
	}

	Parser parser;

	for (auto buf : arrBuffersIn)
	{
		auto bytes = buf->serialize();

		parser.reset();
		auto size = parser.parse(bytes, 0);

		if (size != bytes.size())
		{
			std::cout << "Buffer parser failed: " << std::endl;
			std::cout << "Parsed " << size << ". Expected " << bytes.size() << std::endl;
			return false;
		}

		if (parser.getState() != Parser::State::BUFFER_READY)
		{
			std::cout << "Buffer parser failed: " << std::endl;
			std::cout << "State " << static_cast<int32_t>(parser.getState()) << std::endl;
			return false;
		}

		const auto data = parser.getData();

		bytes.erase(bytes.begin());
		if (bytes != data)
		{
			std::cout << "Buffer parser failed: " << std::endl;

			{
				std::cout << "Data: " << std::endl;
				std::ios state(nullptr);
				state.copyfmt(std::cout);
				for (auto byte : data) {
					std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
				}
				std::cout << std::endl;
				std::cout.copyfmt(state);
			}

			{
				std::cout << "Expected: " << std::endl;
				std::ios state(nullptr);
				state.copyfmt(std::cout);
				for (auto byte : bytes) {
					std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
				}
				std::cout << std::endl;
				std::cout.copyfmt(state);
			}

			return false;
		}
	}

	return true;
}

bool testParser()
{
	bool res = true;

	res = res && testParserFrame();
	res = res && testParserBuffer();

	return res;
}

int main(int argc, char *argv[])
{
	std::cout << "Running" << std::endl;

	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	{
		bool res = testParser();
		std::cout << "Parser: " << bool_to_str(res) << std::endl;
	}

	{
		bool res = testPipeCreate();
		std::cout << "PipeCreate(): " << bool_to_str(res) << std::endl;
	}

	if (argc > 1)
	{
		bool res = testBuffer(true);
		std::cout << "Buffer read: " << bool_to_str(res) << std::endl;
	}
	else
	{
		bool res = testBuffer(false);
		std::cout << "Buffer write: " << bool_to_str(res) << std::endl;
	}

	{
		bool res = testBufferMultithreaded();
		std::cout << "Buffer multithreaded: " << bool_to_str(res) << std::endl;
	}

	if (argc > 1)
	{
		bool res = testFrame(true);
		std::cout << "Frame read: " << bool_to_str(res) << std::endl;
	}
	else
	{
		bool res = testFrame(false);
		std::cout << "Frame write: " << bool_to_str(res) << std::endl;
	}

	if (argc > 1)
	{
		bool res = testMessage(true);
		std::cout << "Message read: " << bool_to_str(res) << std::endl;
	}
	else
	{
		bool res = testMessage(false);
		std::cout << "Message write: " << bool_to_str(res) << std::endl;
	}

	if (argc > 1)
	{
		bool res = testAll(true);
		std::cout << "Test all read: " << bool_to_str(res) << std::endl;
	}
	else
	{
		bool res = testAll(false);
		std::cout << "Test all write: " << bool_to_str(res) << std::endl;
	}

	return 0;
}
