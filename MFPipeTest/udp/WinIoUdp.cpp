#include "WinIoUdp.hpp"

#include <iostream>

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
		std::cout << "WSA Startup failed " << res << std::endl;
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
	if (mode == Mode::WRITE)
	{
		return true;
	}
	else if (mode == Mode::READ)
	{
		if (!create(id))
		{
			std::cout << "Failed to create socket" << std::endl;
			return false;
		}

		if (bind(fd, reinterpret_cast<SOCKADDR *>(&addr), sizeof(addr)) != 0)
		{
			std::cout << "Bind failed" << std::endl;
			return false;
		}

		int32_t timeout = 1;
		auto res = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
		if (res != 0)
		{
			std::cout << "Failed to set SO_RCVTIMEO" << std::endl;
			return false;
		}

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

//	if (res == SOCKET_ERROR)
//		std::cout << "UDP write " << WSAGetLastError() << std::endl;

	return res;
}

ssize_t IoUdp::read(uint8_t *buf, size_t size)
{
	auto res = recvfrom(fd, reinterpret_cast<char *>(buf), size,
						0, nullptr, nullptr);

//	if (res == SOCKET_ERROR)
//		std::cout << "UDP read " << WSAGetLastError() << std::endl;

	return res;
}
