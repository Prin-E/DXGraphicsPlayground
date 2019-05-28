#include "../Common/Win32App.h"
#include <string>

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	string title = u8"Simple";
	Win32App app(title);
	app.createWindow(800, 600);
	app.show();
	app.messageLoop();
}