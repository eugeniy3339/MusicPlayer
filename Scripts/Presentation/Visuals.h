#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

#include <array>

namespace MusicPlayer {
	class Visuals {
	public:
		Visuals(FilesContainer& files, Player& player);
		int Run();

	private:
		FilesContainer& files;
		Player& player;
		bool menuOpen = false;
		bool settingsOpen = false;
		bool lyricsOpen = false;
		bool showCoverArt = false;
		bool invalidMusicPath = false;
		std::array<char, 1024> musicPathInput{};
	};
}
