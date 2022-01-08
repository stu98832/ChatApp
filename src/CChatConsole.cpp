#include "CChatConsole.h"

CChatConsole::CChatConsole()
{
	this->handle = GetStdHandle(STD_OUTPUT_HANDLE);
	this->Resize();
}

void CChatConsole::CleanRange(int x, int y, int w, int h)
{
	CHAR_INFO *chinfos;
	SMALL_RECT rect = { 
		(short)(this->offsetx + x), 
		(short)(this->offsety + y),
		(short)(this->offsetx + x + w),
		(short)(this->offsety + y + h)
	};

	chinfos = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * w * h);

	for (int i = 0; i < w*h; ++i) {
		chinfos[i].Char.AsciiChar = ' ';
		chinfos[i].Attributes     = (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	}

	WriteConsoleOutputA(this->handle, chinfos, { (short)w, (short)h }, { 0, 0 }, &rect);
	delete chinfos;
}

void CChatConsole::PrintString(int x, int y, const char *text)
{
	CHAR_INFO  ch[2];
	SMALL_RECT rect;

	// white text
	ch[0].Attributes = (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	ch[1].Attributes = (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	rect.Top    = this->offsety + y;
	rect.Bottom = this->offsety + y;
	while (*text) {
		rect.Left = this->offsetx + x;
		if (IsDBCSLeadByte(*text)) {
			ch[0].Char.AsciiChar = *text++;
			ch[1].Char.AsciiChar = *text++;
			x += 2;
		} else {
			ch[0].Char.AsciiChar = *text++;
			x += 1;
		}
		rect.Right = this->offsetx + x;
		WriteConsoleOutputA(this->handle, ch, { rect.Right - rect.Left, 1 }, { 0, 0 }, &rect);
	}
}

void CChatConsole::PrintChar(int x, int y, char ch)
{
	CHAR_INFO  chinfo;
	SMALL_RECT rect;

	rect.Left   = this->offsetx + x;
	rect.Right  = this->offsetx + x;
	rect.Top    = this->offsety + y;
	rect.Bottom = this->offsety + y;

	// white text
	chinfo.Attributes = (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	chinfo.Char.AsciiChar = ch;

	WriteConsoleOutputA(this->handle, &chinfo, { 1, 1 }, { 0, 0 }, &rect);
}

void CChatConsole::Resize()
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	GetConsoleScreenBufferInfo(this->handle, &info);
	//this->offsetx = info.srWindow.Left;
	//this->offsety = info.srWindow.Top;
	this->offsetx = 0;
	this->offsety = 0;
	this->width   = info.srWindow.Right  - info.srWindow.Left + 1;
	this->height  = info.srWindow.Bottom - info.srWindow.Top  + 1;
	this->MoveCursor(0, 0);
}

void CChatConsole::MoveCursor(int x, int y)
{
	SetConsoleCursorPosition(this->handle, { (short)x, (short)y });
}

int CChatConsole::GetWidth()
{
	return this->width;
}

int CChatConsole::GetHeight()
{
	return this->height;
}
