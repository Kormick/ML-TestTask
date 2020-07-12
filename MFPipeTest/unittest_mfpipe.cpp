#include "tests/Parser.hpp"
#include "tests/Pipe.hpp"
#include "tests/Udp.hpp"
#include <iostream>
#include <algorithm>

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
	bool profilerRead = argExists(argv, argv + argc, "profilerR");
	bool profilerWrite = argExists(argv, argv + argc, "profilerW");
	bool udp = argExists(argv, argv + argc, "udp");

	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	if (parser)
	{
		bool res = testParser();
		std::cout << "Parser: " << bool_to_str(res) << std::endl;
	}

	if (readProcess)
	{
		if (udp)
		{
			bool res = testUdp(true);
			std::cout << "PipeUdp READ: " << bool_to_str(res) << std::endl;
		}
		else
		{
			bool res = testPipeProcess(true);
			std::cout << "PipeProcess READ: " << bool_to_str(res) << std::endl;
		}
	}

	if (writeProcess)
	{
		if (udp)
		{
			bool res = testUdp(false);
			std::cout << "PipeUdp WRITE: " << bool_to_str(res) << std::endl;
		}
		else
		{
			bool res = testPipeProcess(false);
			std::cout << "PipeProcess WRITE: " << bool_to_str(res) << std::endl;
		}
	}

	if (multithreaded)
	{
		if (udp)
		{
			bool res = testUdpMultithreaded();
			std::cout << "UdpMultithreaded: " << bool_to_str(res) << std::endl;
		}
		else
		{
			bool res = testPipeMultithreaded();
			std::cout << "PipeMultithreaded: " << bool_to_str(res) << std::endl;
		}
	}

	if (profilerRead)
	{
		bool res = testPipePerformance(true);
		std::cout << "PipeProfiler READ: " << bool_to_str(res) << std::endl;
	}

	if (profilerWrite)
	{
		bool res = testPipePerformance(false);
		std::cout << "PipeProfiler WRITE: " << bool_to_str(res) << std::endl;
	}

	return 0;
}
