#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "Commands.h"

namespace MusicPlayer {
	class CommandsListener {
	private:
		std::map<std::string, std::unique_ptr<Command>> commands;
		std::string currentCommand;

	public:
		CommandsListener();

		void RegisterCommand(std::string name, std::unique_ptr<Command> command);
		void Update();
	};
}
