#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

#include <cstddef>
#include <string>
#include <vector>

namespace MusicPlayer {
	class Visuals {
	private:
		Player& player;
		FilesContainer& files;
		std::size_t selectedFile = 0;
		bool isRunning = true;
		bool enteringPath = false;
		bool enteringSearch = false;
		std::string pathInput;
		std::wstring searchInput;
		std::wstring currentTrackPath;
		std::string statusMessage;
		bool isDirty = true;
		int lastConsoleWidth = 0;
		int lastConsoleHeight = 0;

		void HandleKey(int key);
		void Render();
		void PlaySelected();
		std::vector<std::size_t> GetMatchingFiles();

	public:
		Visuals(Player& player, FilesContainer& files);
		bool Update();
	};
}
