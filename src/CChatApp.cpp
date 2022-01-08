#include "CChatApp.h"
#include <conio.h>

#define KEY_ALT_EXTEND 0x00
#define KEY_CTRL_C     0x03
#define KEY_BACKSPACE  0x08
#define KEY_TAB        0x09
#define KEY_ENTER      0x0D
#define KEY_CTRL_R     0x12
#define KEY_HOME       0x47
#define KEY_UP         0x48
#define KEY_PAGE_UP    0x49
#define KEY_LEFT       0x4B
#define KEY_RIGHT      0x4D
#define KEY_END        0x4F
#define KEY_DOWN       0x50
#define KEY_PAGE_DOWN  0x51
#define KEY_INSERT     0x52
#define KEY_DELETE     0x53
#define KEY_EXTEND     0xE0

#define MESSAGE_CHANNEL "5000"
#define AUDIO_CHANNEL   "5001"

#define SERVER 1
#define CLIENT 2

CChatApp::CChatApp() {
	// APP
	this->ResizeConsole();
	this->running              = TRUE;
	this->messageReceiveThread = NULL;
	this->audioReceiveThread   = NULL;
	this->audioSendThread      = NULL;
	this->statusMessage        = "";
	this->messageSocket.SetDisconnectEvent(CChatApp::OnDisconnect, this);
	this->audioSocket.SetDisconnectEvent(CChatApp::OnDisconnect, this);
	this->audio.Initialize(44100, 10.0);

	// INPUT BOX
	this->cursorX       = 0;
	this->maxBufferSize = this->console.GetWidth() - 2;
	this->buffer        = new char[this->maxBufferSize + 1];
	memset(this->buffer, 0, this->maxBufferSize + 1);

	// MESSAGE VIEW
	this->messageOffset = 0;
}

CChatApp::~CChatApp()
{
	this->running = false;
	this->messageSocket.Close();
	this->audioSocket.Close();
	if (this->messageReceiveThread) {
		WaitForSingleObject(this->messageReceiveThread, INFINITE);
		CloseHandle(this->messageReceiveThread);
	}
	if (this->audioReceiveThread) {
		WaitForSingleObject(this->audioReceiveThread, INFINITE);
		CloseHandle(this->audioReceiveThread);
	}
	delete this->buffer;
	this->console.MoveCursor(0, this->console.GetHeight()-1);
}

bool CChatApp::IsRunning()
{
	return this->running;
}

void CChatApp::Listen(std::string ip)
{
	this->socketType = SERVER;
	this->ip         = ip;
	this->messageReceiveThread = CreateThread(0, 0, CChatApp::SocketReceive, this, 0, 0);
	this->audioReceiveThread   = CreateThread(0, 0, CChatApp::AudioReceive, this, 0, 0);
	this->audioSendThread      = CreateThread(0, 0, CChatApp::OnSendAudio, this, 0, 0);
}

void CChatApp::Connect(std::string ip)
{
	this->socketType = CLIENT;
	this->ip         = ip;
	this->messageReceiveThread = CreateThread(0, 0, CChatApp::SocketReceive, this, 0, 0);
	this->audioReceiveThread   = CreateThread(0, 0, CChatApp::AudioReceive, this, 0, 0);
	this->audioSendThread      = CreateThread(0, 0, CChatApp::OnSendAudio, this, 0, 0);
}

void CChatApp::Refresh()
{
	this->RefreshBorder();
	this->RefreshMessage();
	this->RefreshHistory();
	this->RefreshInputBox();
	this->RefreshStatusBar();
	this->RefreshCursor();
}

void CChatApp::ProcessInput()
{
	int input = _getch();

	if (input < 0) {
		return;
	}

	if (input == KEY_EXTEND) {
		this->ProcessExtendKey();
		return;
	}

	switch (input) {
	case KEY_BACKSPACE:
		this->OnDeleteChar(true);
		break;
	case KEY_ENTER:
		this->OnSendMessage();
		break;
	case KEY_CTRL_C:
		this->running = FALSE;
		break;
	case KEY_TAB:
		this->ResizeConsole();
		this->Refresh();
		break;
	case KEY_CTRL_R:
		this->OnSwitchRecord();
		break;
	case KEY_ALT_EXTEND: {
		char buffer[512];
		input = _getch();
		sprintf(buffer, "Invalid alt extend key: 0x%02X", input);
		this->SetStatusMessage(buffer);
		break;
	}
	default:
		char chars[2] = { (char)input, 0 };
		if (input > 31 && input < 127) {
			this->OnEnterChar(chars, 1);
		} else if (IsDBCSLeadByte((BYTE)input)) {
			chars[1] = (char)_getch();
			this->OnEnterChar(chars, 2);
		} else {
			char buffer[512];
			sprintf(buffer, "Invalid key: 0x%02X", input);
			this->SetStatusMessage(buffer);
		}
	}
}

void CChatApp::SetStatusMessage(std::string message)
{
	this->statusMessage = message;
	this->RefreshStatusBar();
	this->RefreshCursor();
}

void CChatApp::ProcessExtendKey()
{
	int input;

	input = _getch();
	switch (input) {
	case KEY_UP:
		this->ScrollMessage(this->messageOffset - 1);
		break;
	case KEY_DOWN:
		this->ScrollMessage(this->messageOffset + 1);
		break;
	case KEY_LEFT:
		if (this->cursorX > 1 && IsDBCSLeadByte((BYTE)this->buffer[this->cursorX - 2])) {
			this->MoveCursor(this->cursorX - 2);
		} else {
			this->MoveCursor(this->cursorX - 1);
		}
		break;
	case KEY_RIGHT:
		if (IsDBCSLeadByte((BYTE)this->buffer[this->cursorX])) {
			this->MoveCursor(this->cursorX + 2);
		} else {
			this->MoveCursor(this->cursorX + 1);
		}
		break;
	case KEY_HOME:
		this->MoveCursor(0);
		break;
	case KEY_END:
		this->MoveCursor(strlen(this->buffer));
		break;
	case KEY_DELETE:
		this->OnDeleteChar(false);
		break;
	default:
		char buffer[512];
		sprintf(buffer, "Invalid extend key: 0x%02X", input);
		this->SetStatusMessage(buffer);
		break;
	}
}

void CChatApp::OnSwitchRecord()
{
	if (!this->audio.IsInitialized()) {
		this->SetStatusMessage(this->audio.GetLastErrorText());
		return;
	}
	if (!this->audioSocket.IsConnecting()) {
		this->SetStatusMessage("Audio channel not open");
		return;
	}
	this->audio.SetRecoding(!this->audio.IsRecoding());
	this->SetStatusMessage(this->audio.IsRecoding() ? "mic on!" : "mic off!");
}

DWORD WINAPI CChatApp::OnSendAudio(LPVOID lpParemeter)
{
	CChatApp *app;
	size_t   size;
	float    buffer[4096];

	app = (CChatApp*)lpParemeter;
	while (app->running) {
		if (!app->audioSocket.IsConnecting()) {
			continue;
		}

		size = app->audio.Read(4096, buffer);
		if (size == 0) {
			Sleep(50);
			continue;
		}

		app->audioSocket.Send(&size, sizeof(size_t));
		app->audioSocket.Send(buffer, sizeof(float)*size);
	}
	return 0;
}

void CChatApp::OnSendMessage()
{
	int size;

	size = strlen(this->buffer);
	if (size == 0) {
		return;
	}

	if (this->messageSocket.IsConnecting()) {
		this->messageSocket.Send(&size, sizeof(size_t));
		this->messageSocket.Send(this->buffer, size);

		this->history.push_back(this->buffer);
		while ((int)this->history.size() > this->maxHistorySize) {
			this->history.pop_front();
		}
	}

	this->buffer[0] = '\0';
	this->cursorX   = 0;

	this->RefreshHistory();
	this->RefreshInputBox();
	this->RefreshCursor();
}

void CChatApp::OnEnterChar(char *chars, int length)
{
	int size;

	size = strlen(this->buffer);
	if (size + length < this->maxBufferSize) {
		if (this->cursorX <= size) {
			memmove(this->buffer + this->cursorX + length, this->buffer + this->cursorX, size - this->cursorX + length);
		}
		memcpy(this->buffer + this->cursorX, chars, length);
		this->cursorX += length;
		if (this->cursorX > size) {
			this->buffer[this->cursorX] = '\0';
		}
		this->RefreshInputBox();
		this->RefreshCursor();
	}
}

void CChatApp::OnDeleteChar(bool backspace)
{
	int  size;
	bool check;
	int  charsize = 1;

	size  = strlen(this->buffer);
	check = backspace ? (this->cursorX > 0) : (this->cursorX < size);
	if (check) {
		size = strlen(this->buffer);
		if (backspace) {
			--this->cursorX;
			if (this->cursorX > 0 && IsDBCSLeadByte((BYTE)this->buffer[this->cursorX-1])) {
				--this->cursorX;
			}
		}
		if (IsDBCSLeadByte((BYTE)this->buffer[this->cursorX])) {
			charsize = 2;
		}
		memmove(
			this->buffer + this->cursorX,
			this->buffer + this->cursorX + charsize,
			size - this->cursorX
		);
		this->RefreshInputBox();
		this->RefreshCursor();
	}
}

void CChatApp::ScrollMessage(int offset)
{
	int maxoff;
	int newOffset;

	maxoff    = max(0, (int)this->message.size() - this->maxMessageDisplay);
	newOffset = min(max(0, offset), maxoff);
	if (newOffset == this->messageOffset) {
		return;
	}
	this->messageOffset = newOffset;
	this->RefreshMessage();
	this->RefreshCursor();
}

void CChatApp::MoveCursor(int offset)
{
	int size;

	size = strlen(this->buffer); 
	this->cursorX = max(0, min(size, offset));
	this->RefreshCursor();
}

void CChatApp::OnReceiveMesage(std::string message)
{
	int maxDisplayOffset;

	this->message.push_back(message);
	while ((int)this->message.size() > this->maxMessageSize) {
		this->message.pop_front();
	}

	maxDisplayOffset = this->message.size() - this->maxMessageDisplay;
	if (this->messageOffset + 1 == maxDisplayOffset) {
		++this->messageOffset;
	}

	this->RefreshMessage();
	this->RefreshCursor();
}

void CChatApp::OnDisconnect(CTcpSocket *socket, void *userdata)
{
	CChatApp *app;

	app = (CChatApp*)userdata;

	app->SetStatusMessage("disconnect...");
}

DWORD WINAPI CChatApp::SocketReceive(LPVOID lpParemeter)
{
	CChatApp *app;
	int       size;
	char      buffer[512];
	bool      result;
	
	app = (CChatApp*)lpParemeter;

	app->SetStatusMessage(app->socketType == CLIENT ? "connecting..." : "listening...");
	if (app->socketType == CLIENT) {
		result = app->messageSocket.ConnectTo(app->ip.c_str(), MESSAGE_CHANNEL);
	} else {
		result = app->messageSocket.AcceptFrom(app->ip.c_str(), MESSAGE_CHANNEL);
	}

	if (!result) {
		app->SetStatusMessage("connect failed!");
	}

	app->SetStatusMessage("connected!");
	while (app->running) {
		if (!app->messageSocket.IsConnecting()) {
			Sleep(100);
			continue;
		}
		app->messageSocket.Recv(&size, sizeof(int));
		app->messageSocket.Recv(buffer, size);
		if (size == 0) {
			continue;
		}
		buffer[size] = 0;

		app->OnReceiveMesage(buffer);
	}
	return 0;
}

DWORD WINAPI CChatApp::AudioReceive(LPVOID lpParemeter)
{
	CChatApp *app;
	size_t    size;
	size_t    block_size;
	float     audio_buffer[4096];

	app = (CChatApp*)lpParemeter;

	if (app->socketType == CLIENT) {
		app->audioSocket.ConnectTo(app->ip.c_str(), AUDIO_CHANNEL);
	} else {
		app->audioSocket.AcceptFrom(app->ip.c_str(), AUDIO_CHANNEL);
	}
	while (app->running) {
		if (!app->audioSocket.IsConnecting()) {
			Sleep(100);
			continue;
		}
		app->audioSocket.Recv(&size, sizeof(size_t));

		while (size > 0) {
			block_size = min(4096, size);
			app->audioSocket.Recv(audio_buffer, sizeof(float)*block_size);
			app->audio.Write(block_size, audio_buffer);
			size -= block_size;
		}
	}
	return 0;
}

void CChatApp::ResizeConsole()
{
	console.Resize();

	// SCREEN
	this->splitterY = this->console.GetHeight() / 2;

	// MESSAGE VIEW
	this->maxMessageSize = 100;
	this->maxMessageDisplay = this->console.GetHeight() - this->splitterY - 3;

	// HISTORY VIEW
	this->maxHistorySize = 100;
	this->maxHistoryDisplay = this->splitterY - 2;
}

void CChatApp::RefreshBorder()
{
	int lines[] = { 0, this->splitterY, this->console.GetHeight() - 2};
	for (int y = 0; y < this->console.GetHeight() - 1; ++y) {
		this->console.PrintChar(0, y, '|');
		this->console.PrintChar(this->console.GetWidth() - 1, y, '|');
	}
	for (int y : lines) {
		this->console.PrintChar(0,y, '+');
		for (int x = 1; x < this->console.GetWidth() - 1; ++x) {
			this->console.PrintChar(x, y, '-');
		}
		this->console.PrintChar(this->console.GetWidth() - 1, y, '+');
	}
}

void CChatApp::RefreshMessage()
{
	std::string buf;

	this->console.CleanRange(1, 1 + this->splitterY, this->console.GetWidth() - 2, this->maxMessageDisplay);
	auto iter = this->message.begin();
	std::advance(iter, this->messageOffset);
	for (
		int i = 0; 
		iter != this->message.end()
		&& i < this->maxMessageDisplay;
		++i, 
		++iter) {
		buf = iter->substr(0, min(this->console.GetWidth() - 2, (int)iter->size()));
		this->console.PrintString(1, 1 + this->splitterY + i, buf.c_str());
	}
}

void CChatApp::RefreshHistory()
{
	std::string buf;

	this->console.CleanRange(1, 1, this->console.GetWidth() - 2, this->maxHistoryDisplay);
	auto iter = this->history.begin();
	std::advance(iter, max(0, (int)this->history.size() - this->maxHistoryDisplay));
	for (
		int i = 0;
		iter != this->history.end()
		&& i < this->maxHistoryDisplay;
		++i,
		++iter) {
		buf = iter->substr(0, min(this->console.GetWidth() - 2, (int)iter->size()));
		this->console.PrintString(1, 1 + i, buf.c_str());
	}
}

void CChatApp::RefreshInputBox()
{
	int         cursorY;
	std::string buf;

	cursorY = min(1 + (int)this->history.size(), this->splitterY - 1);
	buf     = std::string(this->buffer);
	buf     = buf.substr(0, min(this->console.GetWidth() - 2, (int)buf.size()));
	this->console.CleanRange(1, cursorY, this->console.GetWidth() - 2, 1);
	this->console.CleanRange(1, this->splitterY - 1, this->console.GetWidth() - 2, 1); // for last row
	this->console.PrintString(1, cursorY, buf.c_str());
}

void CChatApp::RefreshStatusBar()
{
	std::string buf;

	buf = this->statusMessage.substr(0, max(this->console.GetWidth() - 1, (int)this->statusMessage.size()));
	this->console.CleanRange(0, this->console.GetHeight() - 1, this->console.GetWidth(), 1);
	this->console.PrintString(0, this->console.GetHeight() - 1, buf.c_str());
}

void CChatApp::RefreshCursor()
{
	int cursorY;

	cursorY = min(1 + (int)this->history.size(), this->splitterY - 1);
	this->console.MoveCursor(1 + this->cursorX, cursorY);
}
