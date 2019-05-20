#include "UdpSocket.h"

#include <cstddef>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace net::ip {

template<typename IpAddressType>
typename UdpSocket<IpAddressType>::CreationResult UdpSocket<IpAddressType>::create(
	Port port, IpAddressType addr, std::size_t bufSize) {
	int32_t sockfd{BAD_SOCK_FD};
	if constexpr (std::is_same<IpAddressType, IPv4Address>::value) {
		sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	}

	if (BAD_SOCK_FD == sockfd) {
		return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
	}

	const auto sockAddr = sockAddress({port, addr});
	if (ERROR_CODE == bind(sockfd, reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr))) {
		return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
	}

	SockAddress realAddr{};
	socklen_t realAddrSize = sizeof(realAddr);
	Endpoint<IpAddressType> realEndpoint{};
	if (ERROR_CODE == getsockname(sockfd, reinterpret_cast<sockaddr*>(&realAddr), &realAddrSize)) {
		return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
	}

	if constexpr (std::is_same<IpAddressType, IPv4Address>::value) {
		realEndpoint.m_addr = realAddr.sin_addr.s_addr;
		realEndpoint.m_port = ntohs(realAddr.sin_port);
	} else {
		realEndpoint.m_addr = realAddr.sin6_addr;
		realEndpoint.m_port = ntohs(realAddr.sin6_port);
	}

	return std::unique_ptr<UdpSocket<IpAddressType>>(new UdpSocket{sockfd, realEndpoint, bufSize});
}

template<typename IpAddressType>
nonstd::expected<void, std::error_code> UdpSocket<IpAddressType>::send(const Packet<IpAddressType>& packet) const {
	const auto sockAddr = sockAddress(packet.m_endpoint);
	if (ERROR_CODE ==
		sendto(m_sockfd, packet.m_payload.data(), packet.m_payload.size(), 0,
			   reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr))) {
		return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
	}
	return {};
}

template<typename IpAddressType>
typename UdpSocket<IpAddressType>::ReceiveResult UdpSocket<IpAddressType>::receive() {
	if (auto res = receive(&m_buf); res) {
		if (res.value()) {
			return Packet<IpAddressType>{{m_buf.cbegin(), m_buf.cend()}, res.value().value()};
		}
	} else {
		return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
	}
	return std::nullopt;
}

template<typename IpAddressType>
nonstd::expected<void, std::error_code> UdpSocket<IpAddressType>::setReceiveTimeout(
	const timeval& receiveTimeout) {
	if (ERROR_CODE == setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &receiveTimeout, sizeof(receiveTimeout))) {
		return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
	}
	m_receiveTimeout = receiveTimeout;
	return {};
}

template<typename IpAddressType>
typename UdpSocket<IpAddressType>::ReceiveEndpointResult UdpSocket<IpAddressType>::receive(
	Payload* payload) {
	if (nullptr == payload || 0 == payload->capacity()) {
		return nonstd::make_unexpected(std::error_code{EINVAL, std::system_category()});
	}

	payload->resize(payload->capacity());
	SockAddress sockAddr{};
	socklen_t addrSize = sizeof(sockAddr);
	const auto numBytes = recvfrom(m_sockfd, payload->data(), payload->size(), 0,
								   reinterpret_cast<sockaddr*>(&sockAddr), &addrSize);
	if (-1 == numBytes) {
		payload->resize(0);
		if (EAGAIN != errno && EWOULDBLOCK != errno && EINTR != errno) {
			return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
		}
	} else if (0 < numBytes) {
		payload->resize(numBytes);
		if constexpr (std::is_same<IpAddressType, net::ip::IPv4Address>::value) {
			return Endpoint<IpAddressType>{ntohs(sockAddr.sin_port), sockAddr.sin_addr.s_addr};
		} else {
			return Endpoint<IpAddressType>{ntohs(sockAddr.sin6_port), sockAddr.sin6_addr};
		}
	} else {
		payload->resize(0);
	}
	return std::nullopt;
}

template<typename IpAddressType>
typename UdpSocket<IpAddressType>::SockAddress UdpSocket<IpAddressType>::sockAddress(
	const Endpoint<IpAddressType>& endpoint) {
	SockAddress sockAddr{};
	if constexpr (std::is_same<IpAddressType, net::ip::IPv4Address>::value) {
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons(endpoint.m_port);
		sockAddr.sin_addr.s_addr = endpoint.m_addr;
	} else {
		sockAddr.sin6_family = AF_INET6;
		sockAddr.sin6_port = htons(endpoint.m_port);
		sockAddr.sin6_addr = endpoint.m_addr;
	}
	return sockAddr;
}

template<typename IpAddressType>
nonstd::expected<void, std::error_code> UdpSocket<IpAddressType>::close() {
	if (BAD_SOCK_FD != m_sockfd) {
		if (ERROR_CODE == ::close(m_sockfd)) {
			return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
		}
		m_sockfd = BAD_SOCK_FD;
	}
	return {};
}

template<typename IpAddressType>
UdpSocket<IpAddressType>::~UdpSocket() {
	if (auto res = close(); !res) {
		// Do logging here
	}
}

template class UdpSocket<IPv4Address>;
template class UdpSocket<IPv6Address>;

}  // namespace
