#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

namespace MusicPlayer {
	class Visuals {
	public:
		Visuals(Player& player, FilesContainer& data);

	private:
		Player& player;
		FilesContainer& data;

		void Run();
	};
}
