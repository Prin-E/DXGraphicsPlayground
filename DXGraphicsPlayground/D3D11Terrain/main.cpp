#include "../Common/Win32App.h"
#include "TerrainRenderer.h"
#include <string>

int main(int argc, char** argv) {
	string title = u8"Simple";

	Win32App app(title);
	app.setRenderer(new TerrainRenderer());
	app.createWindow(640, 480);
	app.show();
	return app.messageLoop();
}