#pragma once

#include "../Data/FilesContainer.h"
#include "../Data/Saver.h"
#include "../Logic/Player.h"

#include <array>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace MusicPlayer {
	class Visuals {
	public:
		Visuals(FilesContainer& files, Player& player, Saver& saver);
		int Run();

	private:
		FilesContainer& files;
		Player& player;
		Saver& saver;
		bool menuOpen = false;
		bool settingsOpen = false;
		bool lyricsOpen = false;
		bool showCoverArt = false;
		bool invalidMusicPath = false;
		std::array<char, 1024> musicPathInput{};
		std::array<char, 512> searchInput{};
		std::unordered_set<std::filesystem::path> searchResults;
		std::string lastSearchQuery;
		bool searchResultsDirty = true;
	};
}
