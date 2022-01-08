#include "CTcpSocket.h"

CTcpSocket::CTcpSocket()
{
	this->address  = nullptr;
	this->acceptor = INVALID_SOCKET;
	this->sock     = INVALID_SOCKET;

	this->callback = nullptr;
	this->callbackUserData = nullptr;

	this->initialized = !WSAStartup(MAKEWORD(2, 2), &this->wsa);
}

CTcpSocket::~CTcpSocket()
{
	if (this->acceptor != INVALID_SOCKET) {
		closesocket(this->acceptor);
	}
	if (this->sock != INVALID_SOCKET) {
		closesocket(this->sock);
	}
	if (this->address) {
		delete this->address;
	}
	WSACleanup();
}

bool CTcpSocket::IsConnecting()
{
	return this->initialized && (this->sock != INVALID_SOCKET);
}

bool CTcpSocket::AcceptFrom(std::string ip, std::string port)
{
	int      result;
	
	if (!this->initialized)               goto FAIL_TO_ACCEPT;
	if (this->acceptor != INVALID_SOCKET) goto FAIL_TO_ACCEPT;
	if (this->sock     != INVALID_SOCKET) goto FAIL_TO_ACCEPT;

	this->acceptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->acceptor == INVALID_SOCKET) goto FAIL_TO_ACCEPT;

	this->address = new NetAddress(ip, port);
	if (!this->address->GetInfo()) goto ERROR_ON_PARSE_ADDRESS;

	result = bind(this->acceptor, this->address->GetInfo()->ai_addr, this->address->GetInfo()->ai_addrlen);
	if (result == SOCKET_ERROR) goto ERROR_ON_ACCEPT;

	result = listen(this->acceptor, 10);
	if (result == SOCKET_ERROR) goto ERROR_ON_ACCEPT;

	this->sock = accept(this->acceptor, NULL, NULL);
	if (this->sock == INVALID_SOCKET) goto ERROR_ON_ACCEPT;

	return true;

ERROR_ON_ACCEPT:
	closesocket(this->acceptor);
	this->acceptor = INVALID_SOCKET;
ERROR_ON_PARSE_ADDRESS:
	delete this->address;
	this->address = nullptr;
FAIL_TO_ACCEPT:
	return false;
}

bool CTcpSocket::ConnectTo(std::string  ip, std::string  port)
{
	int      result;

	if (!this->initialized)           goto FAIL_TO_CONNECT;
	if (this->sock != INVALID_SOCKET) goto FAIL_TO_CONNECT;

	this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->sock == INVALID_SOCKET) goto FAIL_TO_CONNECT;

	this->address = new NetAddress(ip, port);
	if (!this->address->GetInfo()) goto ERROR_ON_PARSE_ADDRESS;

	result = connect(this->sock, this->address->GetInfo()->ai_addr, this->address->GetInfo()->ai_addrlen);
	if (result == SOCKET_ERROR) goto ERROR_ON_CONNECT;

	return true;

ERROR_ON_CONNECT:
	closesocket(this->acceptor);
	this->acceptor = INVALID_SOCKET;
ERROR_ON_PARSE_ADDRESS:
	delete this->address;
	this->address = nullptr;
FAIL_TO_CONNECT:
	return false;
}

void CTcpSocket::Close()
{
	this->mutex.Lock();
	if (this->sock != INVALID_SOCKET) {
		closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}
	if (this->acceptor != INVALID_SOCKET) {
		closesocket(this->acceptor);
		this->acceptor = INVALID_SOCKET;
	}
	if (this->callback) {
		this->callback(this, this->callbackUserData);
	}
	this->mutex.Release();
}

void CTcpSocket::Recv(void *data, int size)
{
	int readbytes = 0;
	int result    = 0;

	this->mutex.Lock();
	if (this->sock == INVALID_SOCKET) {
		memset(data, 0, size);
		return;
	}
	this->mutex.Release();

	while (readbytes < size) {
		result = recv(this->sock, ((char*)data) + readbytes, size - readbytes, 0);
		if (result == SOCKET_ERROR || result == 0) {
			memset(data, 0, size);
			this->Close();
			return;
		}
		readbytes += result;
	}
}

void CTcpSocket::Send(void *data, int size)
{
	int sentbytes = 0;
	int result    = 0;

	this->mutex.Lock();
	if (this->sock == INVALID_SOCKET) {
		return;
	}
	this->mutex.Release();

	while (sentbytes < size) {
		result = send(this->sock, ((char*)data) + sentbytes, size - sentbytes, 0);
		if (result == SOCKET_ERROR || result == 0) {
			this->Close();
			return;
		}
		sentbytes += result;
	}
}

void CTcpSocket::SetDisconnectEvent(TcpDisconnectCallback callback, void *userdata)
{
	this->callbackUserData = userdata;
	this->callback         = callback;
}

NetAddress::NetAddress(std::string ip, std::string service)
{
	ADDRINFO hint;

	ZeroMemory(&hint, sizeof(ADDRINFO));
	hint.ai_family   = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	getaddrinfo(ip.c_str(), service.c_str(), &hint, &this->info);
	this->ip      = ip;
	this->service = service;
}

NetAddress::~NetAddress()
{
	if (this->info) {
		freeaddrinfo(this->info);
	}
}

std::string NetAddress::IP()
{
	return this->ip;
}

std::string NetAddress::Port()
{
	return this->service;
}

const ADDRINFO *NetAddress::GetInfo()
{
	return this->info;
}
