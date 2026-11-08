#pragma once

#include "MusicPlayerView.h"

namespace MusicPlayer {
	class Visuals {
	private:
		MusicPlayerView view;
		bool running = false;
		bool initialized = false;
	public:
		Visuals(Player& player, FilesContainer& files);
		~Visuals();
		bool Update();
	};
}
