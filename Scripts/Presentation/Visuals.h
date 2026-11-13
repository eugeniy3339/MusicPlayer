#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

namespace MusicPlayer {
	class Visuals {
	public:
		Visuals(FilesContainer& files, Player& player);
		int Run();

	private:
		FilesContainer& files;
		Player& player;
	};
}
