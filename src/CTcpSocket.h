#pragma once
#include <string>
#include <winsock2.h>
#include <sys/types.h>
#include <ws2tcpip.h>
#include "CMutex.h"
#pragma comment (lib, "Ws2_32.lib")

class NetAddress {
private:
	std::string ip;
	std::string service;
	ADDRINFO   *info;
public:
	NetAddress(std::string ip, std::string service);
	~NetAddress();

	std::string IP();
	std::string Port();
	const ADDRINFO *GetInfo();
};

class CTcpSocket;
typedef void(*TcpDisconnectCallback)(CTcpSocket *socket, void *userdata);

class CTcpSocket {
private:
	NetAddress *address;
	SOCKET      acceptor;
	SOCKET      sock;
	CMutex      mutex;
	WSADATA     wsa;
	bool        initialized;

	TcpDisconnectCallback callback;
	void                 *callbackUserData;

public:
	CTcpSocket();
	~CTcpSocket();

	bool IsConnecting();

	bool AcceptFrom(std::string ip, std::string port);
	bool ConnectTo(std::string ip, std::string port);
	void Close();

	void Recv(void *data, int size);
	void Send(void *data, int size);

	void SetDisconnectEvent(TcpDisconnectCallback callback, void *userdata);
};