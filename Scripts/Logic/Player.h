#pragma once
#include "../Data/FilesContainer.h"
#include "../Data/Saver.h"
#include "../../external/miniaudio.h"
#include <filesystem>

namespace MusicPlayer {
	class Player {
	private:
		static Player* instance;
		FilesContainer& filesContainer;
		Saver& saver;

		MusicFile* curFile;
		std::filesystem::path curFilePath;
		eventpp::CallbackList<void()>::Handle filesListRefreshedHandle;
		bool isPlaying;
		ma_engine engine;
		ma_sound sound;
		bool engineInitialized;
		bool soundInitialized;

		float volume;
	public:
		explicit Player(FilesContainer& filesContainer, Saver& saver);
		~Player();

		static Player* getInstance();

		MusicFile* getCurFile();
		bool* getIsPlaying();

		void Play(MusicFile& file);
		void Update();
		void PlayNext();
		void PlayPrevious();
		void Pause();
		void Resume();
		void Stop();
		void RebindCurrentFile(const std::filesystem::path& path);

		const float& getVolume() const;
		void setVolume(float volume);

		eventpp::CallbackList<void(MusicFile)> onMusicStarted;
		eventpp::CallbackList<void()> onMusicPaused;
		eventpp::CallbackList<void()> onMusicResumed;

		eventpp::CallbackList<void(float)> onVolumeChanged;

	private:
		void PlayAdjacent(int offset);
	};
}
