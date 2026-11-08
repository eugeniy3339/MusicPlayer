#pragma once

#include "../external/eventpp/callbacklist.h"
#include <map>
#include <string>

namespace MusicPlayer {
	enum class CommandType {
		Play,
		Pause,
		Resume,
		Next,
		Previous,
		Quit
	};

	class CommandsListener {
	private:
		const std::map<std::string, CommandType> commands{
			{"play", CommandType::Play},
			{"pause", CommandType::Pause},
			{"resume", CommandType::Resume},
			{"next", CommandType::Next},
			{"previous", CommandType::Previous},
			{"quit", CommandType::Quit}
		};
		std::string currentCommand;

	public:
		void Update();

		eventpp::CallbackList<void(CommandType)> onCommandEntered;
	};
}
