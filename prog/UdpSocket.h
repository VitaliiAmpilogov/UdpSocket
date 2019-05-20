#pragma once

#include <netinet/in.h>
#include <nonstd/expected.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>   

namespace net::ip {

using IPv4Address = in_addr_t;
using IPv6Address = in6_addr;
using Port = in_port_t;
using Payload = std::vector<std::byte>;

inline constexpr const Port ANY_FREE_PORT{};
inline constexpr const IPv4Address ANY_IPV4_ADDR{};
inline constexpr const IPv6Address ANY_IPV6_ADDR{};
inline constexpr const timeval DEFAULT_TIMEOUT{0, 0};
inline constexpr const std::size_t DEFAULT_BUF_SIZE{4096u};

template<typename IpAddressType>
struct Endpoint {
    Port m_port{};
    IpAddressType m_addr{};
};

template<typename IpAddressType>
struct Packet {
    Packet(Payload payload, Endpoint<IpAddressType> endpoint) :
            m_payload(std::move(payload)),
            m_endpoint(std::move(endpoint)) {}
    Packet() = default;
    Payload m_payload;
    Endpoint<IpAddressType> m_endpoint;
};

template<typename IpAddressType>
class UdpSocket {
public:
	using ReceiveResult = nonstd::expected<std::optional<Packet<IpAddressType>>, std::error_code>;
	using ReceiveEndpointResult = nonstd::expected<std::optional<Endpoint<IpAddressType>>, std::error_code>;
	using CreationResult = nonstd::expected<std::unique_ptr<UdpSocket>, std::error_code>;

	static CreationResult create(Port port, IpAddressType addr, std::size_t bufSize = DEFAULT_BUF_SIZE);
	ReceiveResult receive();
	ReceiveEndpointResult receive(Payload* payload);
	nonstd::expected<void, std::error_code> send(const Packet<IpAddressType>& packet) const;
	nonstd::expected<void, std::error_code> setReceiveTimeout(const timeval& receiveTimeout);

	UdpSocket(const UdpSocket&) = delete;
	UdpSocket& operator=(const UdpSocket&) = delete;
	UdpSocket& operator=(UdpSocket&&) = delete;
	UdpSocket(UdpSocket&& _rhs) noexcept :
			m_sockfd(_rhs.m_sockfd),
			m_endpoint(std::move(_rhs.m_endpoint)),
			m_receiveTimeout(std::move(_rhs.m_receiveTimeout)),
			m_buf(std::move(_rhs.m_buf)) {
		_rhs.m_sockfd = BAD_SOCK_FD;
	}

	~UdpSocket();

private:
	using SockAddress =
		typename std::conditional<std::is_same<IpAddressType, IPv4Address>::value,
								  sockaddr_in, sockaddr_in6>::type;
	static constexpr const auto ERROR_CODE = -1;
	static constexpr const auto BAD_SOCK_FD = -1;
	static SockAddress sockAddress(const Endpoint<IpAddressType>& endpoint);

	UdpSocket(int32_t sockfd, Endpoint<IpAddressType> endpoint, std::size_t bufSize) :
			m_sockfd(sockfd),
			m_endpoint(std::move(endpoint)),
			m_buf(bufSize) {}
	nonstd::expected<void, std::error_code> close();

	int32_t m_sockfd{BAD_SOCK_FD};
	Endpoint<IpAddressType> m_endpoint;
	timeval m_receiveTimeout{DEFAULT_TIMEOUT};
	Payload m_buf;
};

}  // namespace