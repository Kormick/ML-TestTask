#ifndef WINIOUDP_HPP
#define WINIOUDP_HPP

#include "winsock2.h"

#include "IoInterface.hpp"

class IoUdp : public IoInterface
{
public:
	IoUdp();
	~IoUdp() override;

	bool create(const std::string &id) override;
	bool open(const std::string &id, Mode mode, int32_t timeoutMs = 1000) override;
	bool close() override;
	ssize_t read(uint8_t *buf, size_t size) override;
	ssize_t write(const uint8_t *buf, size_t size) override;

private:
	SOCKET fd;
	struct sockaddr_in addr;
};

#endif // WINIOUDP_HPP
