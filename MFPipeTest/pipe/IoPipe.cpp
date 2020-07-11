#include "IoPipe.hpp"

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

bool IoPipe::create(const std::string &_pipeId)
{
	if (_pipeId.empty())
		return false;

	remove(_pipeId.c_str());
	if (mkfifo(_pipeId.c_str(), 0777) != 0)
		return false;

	pipeId = _pipeId;

	return true;
}

bool IoPipe::open(const std::string &_pipeId, Mode mode)
{
	if (fd != -1 || _pipeId.empty())
		return false;

	if (mode == Mode::READ)
	{
		fd = ::open(_pipeId.c_str(), O_RDONLY);
		return fd != -1;
	}
	else if (mode == Mode::WRITE)
	{
		fd = ::open(_pipeId.c_str(), O_WRONLY);
		return fd != -1;
	}

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

