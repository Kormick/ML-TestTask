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
