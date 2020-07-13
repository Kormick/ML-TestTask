#include "UnixIoUdp.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include "memory.h"
#include "unistd.h"

IoUdp::IoUdp()
	: fd(-1),
	  addrinfo(nullptr)
{}

IoUdp::~IoUdp()
{
	if (fd != -1)
		close();
}

bool IoUdp::create(const std::string &id)
{
	if (id.empty())
		return false;

	const std::string ip_addr = id.substr(id.find_last_of("/") + 1, id.find_last_of(":") - id.find_last_of("/") - 1);
	const std::string port = id.substr(id.find_last_of(":") + 1);

	if (ip_addr.empty() || port.empty())
		return false;

	struct addrinfo hints;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	auto res = getaddrinfo(ip_addr.c_str(), port.c_str(), &hints, &addrinfo);
	if (res != 0 || addrinfo == nullptr)
	{
		std::cout << gai_strerror(res) << std::endl;
		return false;
	}

	fd = socket(addrinfo->ai_family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == -1)
	{
		freeaddrinfo(addrinfo);
		return false;
	}

	int32_t bufSize = 512 * 1024;
	res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(bufSize));
	if (res != 0)
	{
		std::cerr << "Failed to set SO_SNDBUF" << std::endl;
		return false;
	}
	res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufSize, sizeof(bufSize));
	if (res != 0)
	{
		std::cerr << "Failed to set SO_RCVBUF" << std::endl;
		return false;
	}

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
				if (bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) == 0)
					return true;
				freeaddrinfo(addrinfo);
				::close(fd);
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
	if (fd == -1)
		return true;

	freeaddrinfo(addrinfo);
	auto res = ::close(fd);

	if (res != 0)
		return false;

	fd = -1;
	return true;
}

ssize_t IoUdp::write(const uint8_t *buf, size_t size)
{
	size_t bufSize = 32 * 1024;
	size = size > static_cast<size_t>(bufSize) ? bufSize : size;

	return sendto(fd, buf, size, MSG_CONFIRM, addrinfo->ai_addr, addrinfo->ai_addrlen);
}

ssize_t IoUdp::read(uint8_t *buf, size_t size)
{
	return recvfrom(fd, buf, size, MSG_DONTWAIT, nullptr, nullptr);
}
