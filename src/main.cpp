#define _CRT_SECURE_NO_WARNINGS
#include "CChatApp.h"
#include <stdio.h>
#include <string.h>

int main(int argc, const char **argv) {
	CChatApp *app;
	int       isClient;
	int       isServer;

	if (argc < 3) {
		return printf("usage %s client|server ip", argv[0]), 0;
	}

	isClient = strcmp(argv[1], "client") == 0;
	isServer = strcmp(argv[1], "server") == 0;

	if (!isClient && !isServer) {
		return printf("usage %s client|server ip", argv[0]), 0;
	}

	app = new CChatApp();

	if (isClient) {
		app->Connect(argv[2]);
	} else if (isServer) {
		app->Listen(argv[2]);
	}

	app->Refresh();
	while (app->IsRunning()) {
		app->ProcessInput();
	}

	delete app;

	system("pause");
	return 0;
}