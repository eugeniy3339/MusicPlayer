#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

#include <chrono>
#include <cstddef>
#include <string>

namespace MusicPlayer {
	class Visuals {
	private:
		Player& player;
		FilesContainer& files;
		std::size_t selectedFile = 0;
		bool isRunning = true;
		bool enteringPath = false;
		std::string pathInput;
		std::string currentTrackPath;
		std::string statusMessage;
		std::chrono::steady_clock::time_point lastRender{};

		void HandleKey(int key);
		void Render();
		void PlaySelected();

	public:
		Visuals(Player& player, FilesContainer& files);
		bool Update();
	};
}
