#pragma once

#include "MusicPlayerView.h"

namespace MusicPlayer {
	class Visuals {
	private:
		MusicPlayerView view;
	public:
		Visuals(Player& player, FilesContainer& files);
	};
}
