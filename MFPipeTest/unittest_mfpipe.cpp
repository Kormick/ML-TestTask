#include "tests/Parser.hpp"
#include "tests/Pipe.hpp"
#include "tests/Udp.hpp"
#include <iostream>
#include <algorithm>
#include <future>
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
				std::cout << "Write " << i << " " << res << std::endl;
			}
			{
				auto res = MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 1000, "");
				std::cout << "Write " << i << " " << res << std::endl;
			}

bool argExists(char **begin, char **end, const std::string &arg)
{
	return std::find(begin, end, arg) != end;
}

int main(int argc, char *argv[])
{
	bool parser = argExists(argv, argv + argc, "parser");
	bool readProcess = argExists(argv, argv + argc, "read");
	bool writeProcess = argExists(argv, argv + argc, "write");
	bool multithreaded = argExists(argv, argv + argc, "multi");
	bool udp = argExists(argv, argv + argc, "udp");
	bool pipe = argExists(argv, argv + argc, "pipe");
	bool all = argExists(argv, argv + argc, "all");

	if (all)
	{
		parser = true;
		multithreaded = true;
		pipe = true;
		udp = true;
	}

	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	if (parser)
	{
		std::cout << "testParser(): " << std::endl;
		bool res = testParser();
		std::cout << "testParser(): " << bool_to_str(res) << std::endl;
	}

	if (readProcess)
	{
		if (udp)
		{
			std::cout << "testUdp() READ: " << std::endl;
			bool res = testUdp(true);
			std::cout << "testUdp() READ: " << bool_to_str(res) << std::endl;
		}
		if (pipe)
		{
			std::cout << "testPipe() READ: " << std::endl;
			bool res = testPipeProcess(true);
			std::cout << "testPipe() READ: " << bool_to_str(res) << std::endl;
		}
	{
		bool res = testBufferMultithreaded();
		std::cout << "Buffer multithreaded: " << bool_to_str(res) << std::endl;
	}

	if (writeProcess)
	{
		if (udp)
		{
			std::cout << "testUdp() WRITE: " << std::endl;
			bool res = testUdp(false);
			std::cout << "testUdp() WRITE: " << bool_to_str(res) << std::endl;
		}
		if (pipe)
		{
			std::cout << "testPipe() WRITE: " << std::endl;
			bool res = testPipeProcess(false);
			std::cout << "testPipe() WRITE: " << bool_to_str(res) << std::endl;
		}
	}

	if (multithreaded)
	{
		if (udp)
		{
			std::cout << "testUdpMultithreaded(): " << std::endl;
			bool res = testUdpMultithreaded();
			std::cout << "testUdpMultithreaded(): " << bool_to_str(res) << std::endl;
		}
		if (pipe)
		{
			std::cout << "testPipeMultithreaded(): " << std::endl;
			bool res = testPipeMultithreaded();
			std::cout << "testPipeMultithreaded(): " << bool_to_str(res) << std::endl;
		}
	}

	return 0;
}
