#include "UnixIoPipe.hpp"

#include <chrono>
#include <thread>
#include "fcntl.h"
#include "sys/stat.h"
#include "unistd.h"

IoPipe::IoPipe()
	: fd(-1)
{}

IoPipe::~IoPipe()
{
	if (fd != -1)
		close();
}

bool IoPipe::create(const std::string &pipeId)
{
	if (pipeId.empty())
		return false;

	remove(pipeId.c_str());
	if (mkfifo(pipeId.c_str(), 0777) != 0)
		return false;

	return true;
}

bool IoPipe::open(const std::string &pipeId, Mode mode, int32_t timeoutMs /* = 1000 */)
{
	if (fd != -1 || pipeId.empty())
		return false;

	const auto start = std::chrono::steady_clock::now();
	do
	{
		if (mode == Mode::READ)
			fd = ::open(pipeId.c_str(), O_RDONLY | O_NONBLOCK);
		else if (mode == Mode::WRITE)
			fd = ::open(pipeId.c_str(), O_WRONLY | O_NONBLOCK);

		if (fd != -1)
			return true;

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (std::chrono::steady_clock::now() < start + std::chrono::milliseconds(timeoutMs));

	return false;
}

bool IoPipe::close()
{
	if (fd == -1)
		return true;

	return ::close(fd) == 0;
}

ssize_t IoPipe::read(uint8_t *buf, size_t size)
{
	if (fd == -1)
		return -1;

	return ::read(fd, buf, size);
}

ssize_t IoPipe::write(const uint8_t *buf, size_t size)
{
	if (fd == -1)
		return -1;

	return ::write(fd, buf, size);
}

