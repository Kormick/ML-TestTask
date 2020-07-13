#ifndef PIPEINTERFACE_HPP
#define PIPEINTERFACE_HPP

#include <string>

class IoInterface
{
public:
	enum class Mode
	{
		READ = 0x00,
		WRITE = 0x01,
	};

	virtual ~IoInterface() = default;

	virtual bool create(const std::string &pipeId) = 0;
	virtual bool open(const std::string &pipeId, Mode mode, int32_t timeoutMs = 1000) = 0;
	virtual bool close() = 0;
	virtual ssize_t read(uint8_t *buf, size_t size) = 0;
	virtual ssize_t write(const uint8_t *buf, size_t size) = 0;
};

#endif // PIPEINTERFACE_HPP
