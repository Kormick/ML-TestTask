#ifndef UNIXIOUDP_HPP
#define UNIXIOUDP_HPP

#include "arpa/inet.h"
#include "netdb.h"
#include "sys/socket.h"

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
	int32_t fd;
	sockaddr_in addr;
	struct addrinfo *addrinfo;
};

#endif // UNIXIOUDP_HPP
