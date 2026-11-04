#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

namespace MusicPlayer {
	class Visuals {
	public:
		Visuals(Player& player, Data& data);

	private:
		Player& player;
		Data& data;

		void Run();
	};
}
