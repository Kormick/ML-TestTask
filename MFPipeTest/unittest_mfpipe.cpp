#include "tests/Parser.hpp"
#include "tests/Pipe.hpp"
#include <iostream>
#include <algorithm>

bool argExists(char **begin, char **end, const std::string &arg)
{
	return std::find(begin, end, arg) != end;
}

int main(int argc, char *argv[])
{
	bool parser = argExists(argv, argv + argc, "p");
	bool readProcess = argExists(argv, argv + argc, "r");
	bool writeProcess = argExists(argv, argv + argc, "w");
	bool multithreaded = argExists(argv, argv + argc, "m");

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
		bool res = testPipeProcess(true);
		std::cout << "PipeProcess READ: " << bool_to_str(res) << std::endl;
	}

	if (writeProcess)
	{
		bool res = testPipeProcess(false);
		std::cout << "PipeProcess WRITE: " << bool_to_str(res) << std::endl;
	}

	if (multithreaded)
	{
		bool res = testPipeMultithreaded();
		std::cout << "PipeMultithreaded: " << bool_to_str(res) << std::endl;
	}

	return 0;
}
