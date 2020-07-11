#ifndef UNIXIOPIPE_HPP
#define UNIXIOPIPE_HPP

#include "IoInterface.hpp"

class IoPipe : public IoInterface
{
public:
	IoPipe();
	~IoPipe() override;

	bool create(const std::string &_pipeId) override;
	bool open(const std::string &pipeId, Mode mode) override;
	bool close() override;
	ssize_t read(uint8_t *buf, size_t size) override;
	ssize_t write(const uint8_t *buf, size_t size) override;

private:
	std::string pipeId;
	int32_t fd;
};

#endif // UNIXIOPIPE_HPP
