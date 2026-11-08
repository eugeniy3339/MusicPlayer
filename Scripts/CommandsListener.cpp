#include "CommandsListener.h"

#include <conio.h>
#include <cctype>
#include <iostream>

namespace MusicPlayer {
	void CommandsListener::Update() {
		while (_kbhit()) {
			const int key = _getch();

			if (key == '\r' || key == '\n') {
				std::cout << '\n';
				const auto command = commands.find(currentCommand);
				if (command != commands.end())
					onCommandEntered(command->second);
				else if (!currentCommand.empty())
					std::cout << "Unknown command: " << currentCommand << '\n';

				currentCommand.clear();
				continue;
			}

			if (key == '\b' || key == 127) {
				if (!currentCommand.empty()) {
					currentCommand.pop_back();
					std::cout << "\b \b";
				}
				continue;
			}

			// _getch returns a prefix for special keys; ignore the following code too.
			if (key == 0 || key == 0xE0) {
				_getch();
				continue;
			}

			if (std::isprint(static_cast<unsigned char>(key))) {
				currentCommand.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(key))));
				std::cout << static_cast<char>(key);
			}
		}
	}
}
