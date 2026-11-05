#pragma once

#include "../Logic/Player.h"
#include "../Data/FilesContainer.h"

namespace MusicPlayer {
	class MusicPlayerView {
	private:
		Player& player;
		FilesContainer& files;
		eventpp::CallbackList<void()>::Handle filesChangedHandle;
		eventpp::CallbackList<void(MusicFile)>::Handle musicStartedHandle;
		eventpp::CallbackList<void()>::Handle musicPausedHandle;
		eventpp::CallbackList<void()>::Handle musicResumedHandle;

	public:
		MusicPlayerView(Player& player, FilesContainer& files);
		~MusicPlayerView();

		void Draw();
	};
}
