#include <Windows.h>

class CChatConsole {
protected:
	HANDLE handle;
	int    height;
	int    width;
	int    offsetx;
	int    offsety;
public:
	CChatConsole();

	void CleanRange(int x, int y, int w, int h);
	void PrintString(int x, int y, const char *text);
	void PrintChar(int x, int y, char ch);
	void Resize();
	void MoveCursor(int x, int y);
	int  GetWidth();
	int  GetHeight();
};