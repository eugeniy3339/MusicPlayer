#include "CommandsListener.h"

#include <conio.h>
#include <cctype>
#include <iostream>

namespace MusicPlayer {
	CommandsListener::CommandsListener() {
		RegisterCommand("play", std::make_unique<PlayCommand>());
	}

	void CommandsListener::RegisterCommand(std::string name, std::unique_ptr<Command> command) {
		for (char& character : name)
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

		if (!name.empty() && command)
			commands[std::move(name)] = std::move(command);
	}

	void CommandsListener::Update() {
		while (_kbhit()) {
			const int key = _getch();

			if (key == '\r' || key == '\n') {
				std::cout << '\n';
				const std::size_t firstCharacter = currentCommand.find_first_not_of(" \t");
				if (firstCharacter == std::string::npos) {
					currentCommand.clear();
					continue;
				}

				const std::size_t commandEnd = currentCommand.find_first_of(" \t", firstCharacter);
				std::string commandName = currentCommand.substr(
					firstCharacter, commandEnd == std::string::npos ? commandEnd : commandEnd - firstCharacter);
				for (char& character : commandName)
					character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

				const auto command = commands.find(commandName);
				if (command != commands.end()) {
					const std::size_t attributesStart = commandEnd == std::string::npos
						? currentCommand.size()
						: currentCommand.find_first_not_of(" \t", commandEnd);
					const std::string attributes = attributesStart == std::string::npos
						? std::string{}
						: currentCommand.substr(attributesStart);
					command->second->Start(attributes);
				}
				else if (!currentCommand.empty())
					std::cout << "Unknown command: " << commandName << '\n';

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
				currentCommand.push_back(static_cast<char>(key));
				std::cout << static_cast<char>(key);
			}
		}
	}
}
