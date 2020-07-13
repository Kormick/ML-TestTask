#ifndef UDP_HPP
#define UDP_HPP

#include "tests/Pipe.hpp"

static constexpr auto udpAddr = "udp://127.0.0.10:49152";

bool testUdp(bool read)
{
	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	bool res = true;

	{
		bool inRes = testBuffer(udpAddr, read);
		std::cout << "\ttestUdpBuffer(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testFrame(udpAddr, read);
		std::cout << "\ttestUdpFrame(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testMessage(udpAddr, read);
		std::cout << "\ttestUdpMessage(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testAll(udpAddr, read);
		std::cout << "\ttestUdpAll(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	return res;
}

bool testUdpMultithreaded()
{
	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	bool res = true;

	{
		bool inRes = testBufferMultithreadedOwnThreads(udpAddr);
		std::cout << "\ttestUdpBufferMultithreadedOwnThreads(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testBufferMultithreaded(udpAddr);
		std::cout << "\ttestUdpBufferMultithreaded(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	return res;
}

#endif // UDP_HPP
