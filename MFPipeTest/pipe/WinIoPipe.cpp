#include "WinIoPipe.hpp"

#include <chrono>
#include <iostream>
#include <thread>

IoPipe::IoPipe()
	: fd(INVALID_HANDLE_VALUE)
{}

IoPipe::~IoPipe()
{
	if (fd != INVALID_HANDLE_VALUE)
		close();
}

bool IoPipe::create(const std::string &_pipeId)
{
	if (_pipeId.empty())
		return false;

	fd = CreateNamedPipe(
				_pipeId.c_str(),
				PIPE_ACCESS_DUPLEX,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				65535,
				65535,
				1000,
				NULL);

	std::cout << GetLastError() << std::endl;

	if (fd == INVALID_HANDLE_VALUE)
		return false;

	pipeId = _pipeId;
	return true;
}

bool IoPipe::open(const std::string &_pipeId, Mode mode, int32_t timeoutMs /* = 1000 */)
{
	if (_pipeId.empty())
		return false;

	const auto start = std::chrono::steady_clock::now();

	do
	{
		if (mode == Mode::READ)
		{
			fd = CreateFile(
						_pipeId.c_str(),
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

			if (fd != INVALID_HANDLE_VALUE)
			{
				mode = Mode::READ;
				return true;
			}
		}
		else if (mode == Mode::WRITE)
		{
			auto res = ConnectNamedPipe(fd, NULL);

			auto err = GetLastError();
			if (res || err == ERROR_PIPE_CONNECTED)
			{
				mode = Mode::WRITE;
				return true;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(timeoutMs));

	return false;
}

bool IoPipe::close()
{
	if (fd == INVALID_HANDLE_VALUE)
		return true;

	if (mode == Mode::WRITE)
	{
		FlushFileBuffers(fd);
		DisconnectNamedPipe(fd);
	}

	return CloseHandle(fd);
}

ssize_t IoPipe::read(uint8_t *buf, size_t size)
{
	if (fd == INVALID_HANDLE_VALUE)
		return -1;

	DWORD bytesRead = 0;
	auto res = ReadFile(fd, buf, size, &bytesRead, NULL);

	return res ? bytesRead : static_cast<ssize_t>(-1);
}

ssize_t IoPipe::write(const uint8_t *buf, size_t size)
{
	if (fd == INVALID_HANDLE_VALUE)
		return -1;

	DWORD bytesWritten = 0;
	auto res = WriteFile(fd, buf, size, &bytesWritten, NULL);

	return res ? bytesWritten : static_cast<ssize_t>(-1);
}
