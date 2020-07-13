#ifndef WINIOPIPE_HPP
#define WINIOPIPE_HPP

#include "IoInterface.hpp"

#include "windows.h"

class IoPipe : public IoInterface
{
public:
	IoPipe();
	~IoPipe() override;

	bool create(const std::string &_pipeId) override;
	bool open(const std::string &pipeId, Mode mode, int32_t timeoutMs = 1000) override;
	bool close() override;
	ssize_t read(uint8_t *buf, size_t size) override;
	ssize_t write(const uint8_t *buf, size_t size) override;

private:
	std::string pipeId;
	Mode mode;
	HANDLE fd;
};

#endif // WINIOPIPE_HPP
