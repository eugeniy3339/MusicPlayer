#pragma once

#include "../Data/FilesContainer.h"
#include "../Logic/Player.h"

namespace MusicPlayer {
	class MusicPlayerView {
	public:
		MusicPlayerView();
		void Render(FilesContainer& filesContainer, Player& player);

	private:
		char pathBuffer[1024];
		int selectedIndex;

		void ChangePath(FilesContainer& filesContainer, Player& player);
		void PlaySelected(FilesContainer& filesContainer, Player& player);
		void PlayAdjacent(FilesContainer& filesContainer, Player& player, int offset);
	};
}
