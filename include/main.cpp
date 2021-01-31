#include <iostream>
#include "lmqtt.h"
#include "lmqtt_timer_queue.h"

int main() {

	{
		lmqtt::timer_queue q;
	}


	lmqtt::lmqtt_server lmqtt_server(1883);
	if (!lmqtt_server.start()) {
		std::cout << "Error starting server...\n";
		return 0;
	}

	/*bool key[3] = { false,false,false };
	bool old_key[3] = { false,false,false };

	bool quit = false;
	while (!quit) {
		if (GetForegroundWindow() == GetConsoleWindow()) {
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if (key[0] && !old_key[0]) quit = true;
		if (key[1] && !old_key[1]) quit = true;
		if (key[2] && !old_key[2]) quit = true;

		// avoid rebondissement (in french)
		for (int i = 0; i < 3; ++i) old_key[i] = key[i];
		
	}*/

	while (1) {
		lmqtt_server.update();
	}
	
	//system("pause");
	return 0;
}