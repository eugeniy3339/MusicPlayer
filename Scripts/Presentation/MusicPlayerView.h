#pragma once

#include "../Logic/Player.h"
#include "../Data/FilesContainer.h"
#include <memory>
#include <mutex>
#include <string>

namespace MusicPlayer {
	class MusicPlayerView {
	private:
		struct FolderPickerState {
			std::mutex mutex;
			bool running = false;
			bool completed = false;
			std::wstring selectedPath;
		};

		Player& player;
		FilesContainer& files;
		std::shared_ptr<FolderPickerState> folderPickerState;
		eventpp::CallbackList<void()>::Handle filesChangedHandle;
		eventpp::CallbackList<void(MusicFile)>::Handle musicStartedHandle;
		eventpp::CallbackList<void()>::Handle musicPausedHandle;
		eventpp::CallbackList<void()>::Handle musicResumedHandle;

	public:
		MusicPlayerView(Player& player, FilesContainer& files);
		~MusicPlayerView();

		void Draw();

	private:
		void StartFolderPicker();
	};
}
