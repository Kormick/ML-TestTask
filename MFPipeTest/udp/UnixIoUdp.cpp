#include "UnixIoUdp.hpp"

#include <iostream>
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
		std::cout << "Failed to set SO_SNDBUF" << std::endl;
		return false;
	}
	res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufSize, sizeof(bufSize));
	if (res != 0)
	{
		std::cout << "Failed to set SO_RCVBUF" << std::endl;
		return false;
	}

	return true;
}

bool IoUdp::open(const std::string &id, Mode mode, int32_t timeoutMs)
{
	if (mode == Mode::WRITE)
	{
		return true;
	}
	else if (mode == Mode::READ)
	{
		if (!create(id))
			return false;

		if (bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) != 0)
		{
			freeaddrinfo(addrinfo);
			::close(fd);
			return false;
		}

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

	auto res = sendto(fd, buf, size, MSG_CONFIRM, addrinfo->ai_addr, addrinfo->ai_addrlen);
	if (res == -1)
		std::cout << "UDP write: " << strerror(errno) << std::endl;
	return res;
}

ssize_t IoUdp::read(uint8_t *buf, size_t size)
{
	auto res = recvfrom(fd, buf, size, MSG_DONTWAIT, nullptr, nullptr);
//	if (res == -1)
//		std::cout << "UDP read: " << strerror(errno) << std::endl;
	return res;
}
