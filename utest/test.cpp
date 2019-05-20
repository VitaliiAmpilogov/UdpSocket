#include <gtest/gtest.h>
#include "UdpSocket.h"
#include <arpa/inet.h>
#include <chrono>

using namespace std::chrono_literals;
using namespace net::ip;
constexpr const Port PORT_7777 = 7777;
constexpr const std::size_t BUF_SIZE = 1024;
constexpr const auto SRV_ADDR4 = "127.0.0.10";
constexpr const auto SRV_ADDR6 = "::ffff:7f00:000a";
constexpr const auto RESPONSE = "RESPONSE";
constexpr const auto REQUEST = "REQUEST";
constexpr const timeval RCV_TIMEOUT{2, 0};  // 2 seconds
constexpr const auto TIMEOUT_2S = 2000ms;
using Socket_v4 = UdpSocket<IPv4Address>;
using Socket_v6 = UdpSocket<IPv6Address>;

class Timer {
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

public:
	auto stop() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start);
	}
};

template<typename IpAddressType>
std::optional<IpAddressType> stringToIpAddress(const std::string& strAddr) {
	if constexpr (std::is_same<IpAddressType, IPv4Address>::value) {
		IPv4Address ip4Address{};
		if (inet_pton(AF_INET, strAddr.c_str(), &ip4Address) == 1) {
			return ip4Address;
		}
		return std::nullopt;
	} else {
		IPv6Address ip6Address{};
		if (inet_pton(AF_INET6, strAddr.c_str(), &ip6Address) == 1) {
			return ip6Address;
		}
		return std::nullopt;
	}
}

std::vector<std::byte> stringToBytes(const std::string& str) {
	return {reinterpret_cast<const std::byte*>(str.data()),
			reinterpret_cast<const std::byte*>(str.data() + str.size())};
}

std::string bytesToString(const std::vector<std::byte>& bytes) {
	return {reinterpret_cast<const char*>(bytes.data()),
			reinterpret_cast<const char*>(bytes.data() + bytes.size())};
}

TEST(UdpSocket, CreateIpv4SocketWithDefaultParams) {
	auto socket = Socket_v4::create(ANY_FREE_PORT, ANY_IPV4_ADDR).value();
	ASSERT_TRUE(socket);
}

TEST(UdpSocket, CreateIpv6SocketWithDefaultParams) {
	auto socket = Socket_v6::create(ANY_FREE_PORT, ANY_IPV6_ADDR).value();
	ASSERT_TRUE(socket);
}

TEST(UdpSocket, CreateIpv4SocketWithPort_Addr_RcvTimeout) {
	const auto addr = stringToIpAddress<IPv4Address>(SRV_ADDR4).value();
	auto socket = Socket_v4::create(PORT_7777, addr).value();
	ASSERT_TRUE(socket);
    ASSERT_TRUE(socket->setReceiveTimeout(RCV_TIMEOUT));
	Timer timer;
	auto result = socket->receive();
	ASSERT_LE(TIMEOUT_2S, timer.stop());
}

TEST(UdpSocket, CreateIpv6SocketWithPort_Addr_RcvTimeout) {
	const auto addr = stringToIpAddress<IPv6Address>(SRV_ADDR6).value();
	auto socket = Socket_v6::create(PORT_7777, addr).value();
	ASSERT_TRUE(socket);
    ASSERT_TRUE(socket->setReceiveTimeout(RCV_TIMEOUT));
	Timer timer;
	auto result = socket->receive();
	ASSERT_LE(TIMEOUT_2S, timer.stop());
}

TEST(UdpSocket, ReceiveWithNullPayload_Negative) {
	auto socket = Socket_v6::create(PORT_7777, ANY_IPV6_ADDR).value();
	ASSERT_TRUE(socket);
	auto res = socket->receive(nullptr);
	ASSERT_FALSE(res);
	ASSERT_EQ(res.error().value(), EINVAL);
}

TEST(UdpSocket, ReceiveWithSmallPayload_Negavive) {
	auto socket = Socket_v6::create(PORT_7777, ANY_IPV6_ADDR).value();
	ASSERT_TRUE(socket);
	Payload payload;
	payload.shrink_to_fit();
	const auto res = socket->receive(&payload);
	ASSERT_FALSE(res);
	ASSERT_EQ(res.error().value(), EINVAL);
}

TEST(UdpSocket, Ipv4SocketsCommunication) {
	const auto serverAddr = stringToIpAddress<IPv4Address>(SRV_ADDR4).value();
	auto serverSocket = Socket_v4::create(PORT_7777, serverAddr).value();
	auto clientSocket = Socket_v4::create(ANY_FREE_PORT, ANY_IPV4_ADDR).value();
	const auto pktReq = Packet<IPv4Address>{stringToBytes(REQUEST), {PORT_7777, serverAddr}};
	const auto sndReq = clientSocket->send(pktReq);
	ASSERT_TRUE(sndReq); 
	auto request = serverSocket->receive();
	ASSERT_TRUE(request);
	ASSERT_TRUE(request.value());
	const auto pktRes = Packet{stringToBytes(RESPONSE), request.value()->m_endpoint};
	const auto sndRes = serverSocket->send(pktRes);
	ASSERT_TRUE(sndRes);
	const auto response = clientSocket->receive();
	ASSERT_TRUE(response);
	ASSERT_TRUE(response.value());
	const auto answer = bytesToString(response.value()->m_payload);
	ASSERT_EQ(RESPONSE, answer);
	ASSERT_EQ(response.value()->m_endpoint.m_port, PORT_7777);
}

TEST(UdpSocket, Ipv6SocketsCommunication) {
	const auto serverAddr = stringToIpAddress<IPv6Address>(SRV_ADDR6).value();
	auto serverSocket = Socket_v6::create(PORT_7777, serverAddr).value();
	auto clientSocket = Socket_v6::create(ANY_FREE_PORT, ANY_IPV6_ADDR).value();
	const auto pktReq = Packet<IPv6Address>{stringToBytes(REQUEST), {PORT_7777, serverAddr}};
	const auto sndReq = clientSocket->send(pktReq);
	ASSERT_TRUE(sndReq); 
	auto request = serverSocket->receive();
	ASSERT_TRUE(request);
	ASSERT_TRUE(request.value());
	const auto pktRes = Packet{stringToBytes(RESPONSE), request.value()->m_endpoint};
	const auto sndRes = serverSocket->send(pktRes);
	ASSERT_TRUE(sndRes);
	const auto response = clientSocket->receive();
	ASSERT_TRUE(response);
	ASSERT_TRUE(response.value());
	const auto answer = bytesToString(response.value()->m_payload);
	ASSERT_EQ(RESPONSE, answer);
	ASSERT_EQ(response.value()->m_endpoint.m_port, PORT_7777);
}

int32_t main(int32_t argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}