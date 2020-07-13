#include "WinIoUdp.hpp"

#include <chrono>
#include <iostream>
#include <thread>

IoUdp::IoUdp()
	: fd(INVALID_SOCKET)
{}

IoUdp::~IoUdp()
{
	if (fd != INVALID_SOCKET)
		close();
}

bool IoUdp::create(const std::string &id)
{
	if (id.empty())
		return false;

	WSADATA wsaData;
	int32_t res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NO_ERROR)
	{
		std::cerr << "WSA Startup failed " << res << std::endl;
		return false;
	}

	const std::string ip_addr = id.substr(id.find_last_of("/") + 1, id.find_last_of(":") - id.find_last_of("/") - 1);
	const std::string port = id.substr(id.find_last_of(":") + 1);

	if (ip_addr.empty() || port.empty())
		return false;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(std::stoi(port));
	addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == INVALID_SOCKET)
		return false;

	return true;
}

bool IoUdp::open(const std::string &id, Mode mode, int32_t timeoutMs)
{
	if (mode == Mode::READ)
	{
		if (id.empty())
			return false;

		const auto start = std::chrono::steady_clock::now();
		const auto end = start + std::chrono::milliseconds(timeoutMs);

		do
		{
			if (create(id))
			{
				if (bind(fd, reinterpret_cast<SOCKADDR *>(&addr), sizeof(addr)) == 0)
				{
					int32_t timeout = 1;
					if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout)) == 0)
						return true;
				}
				closesocket(fd);
				WSACleanup();
			}
			std::this_thread::yield();
		} while (std::chrono::steady_clock::now() < end);
	}
	else if (mode == Mode::WRITE)
	{
		return true;
	}

	return false;
}

bool IoUdp::close()
{
	if (fd == INVALID_SOCKET)
		return true;

	closesocket(fd);
	WSACleanup();

	return true;
}

ssize_t IoUdp::write(const uint8_t *buf, size_t size)
{
	size_t bufSize = 32 * 1024;
	size = size > bufSize ? bufSize : size;

	auto res = sendto(fd, reinterpret_cast<const char *>(buf), size,
					  0, reinterpret_cast<SOCKADDR *>(&addr), sizeof(addr));
	return res;
}

ssize_t IoUdp::read(uint8_t *buf, size_t size)
{
	auto res = recvfrom(fd, reinterpret_cast<char *>(buf), size,
						0, nullptr, nullptr);
	return res;
}
