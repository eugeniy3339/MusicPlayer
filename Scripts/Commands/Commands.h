#pragma once

#include "../Logic/Player.h"
#include "../Data/FilesContainer.h"

namespace MusicPlayer {
#pragma region Commands
	class Command {
	public:
		virtual ~Command() = default;
		virtual void Start(std::string attributes) = 0;
	};

	class PlayCommand : public Command {
	public:
		void Start(std::string attributes) override;
	};

	class NextCommand : public Command {
	public:
		void Start(std::string attributes) override;
	};

	class PreviousCommand : public Command {
	public:
		void Start(std::string attributes) override;
	};

#pragma endregion
}