#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "CTcpSocket.h"
#include "CCircularQueue.h"
#include "CChatAudio.h"
#include "CChatConsole.h"
#include <Windows.h>
#include <string>
#include <list>

class CChatApp {
private:
	// UI
	int                    cursorX;
	int                    splitterY;
	int                    messageOffset;
	int                    maxMessageSize;
	int                    maxHistorySize;
	int                    maxMessageDisplay;
	int                    maxHistoryDisplay;
	char                  *buffer;
	int                    maxBufferSize;
			              
	// SOCKET             
	CTcpSocket             messageSocket;
	CTcpSocket             audioSocket;
	HANDLE                 messageReceiveThread;
	HANDLE                 audioReceiveThread;
	HANDLE                 audioSendThread;
	int                    socketType;
	std::string            ip;
	std::string            port;

	// APP
	CChatAudio             audio;
	CChatConsole           console;
	bool                   running;
	std::string            statusMessage;
	std::list<std::string> history;
	std::list<std::string> message;
public:
	CChatApp();
	~CChatApp();
	bool IsRunning();
	void Listen(std::string ip);
	void Connect(std::string ip);
	void Refresh();
	void ProcessInput();
	void SetStatusMessage(std::string message);
private:
	void ProcessExtendKey();
	void OnSwitchRecord();
	void OnSendMessage();
	void OnEnterChar(char *chars, int length);
	void OnDeleteChar(bool backspace);
	void ScrollMessage(int offset);
	void MoveCursor(int offset);
	void OnReceiveMesage(std::string message);
	static void OnDisconnect(CTcpSocket *socket, void *userdata);
	static DWORD WINAPI OnSendAudio(LPVOID lpParemeter);
	static DWORD WINAPI SocketReceive(LPVOID lpParemeter);
	static DWORD WINAPI AudioReceive(LPVOID lpParemeter);
	void ResizeConsole();
	void RefreshBorder();
	void RefreshMessage();
	void RefreshHistory();
	void RefreshInputBox();
	void RefreshStatusBar();
	void RefreshCursor();
};