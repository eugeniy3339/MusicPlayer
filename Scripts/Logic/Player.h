#pragma once
#include "../Data/FilesContainer.h"
#include "../Data/Saver.h"
#include "../../external/miniaudio.h"

namespace MusicPlayer {
	class Player {
	private:
		static Player* instance;
		FilesContainer& filesContainer;
		Saver& saver;

		MusicFile* curFile;
		bool isPlaying;
		ma_engine engine;
		ma_sound sound;
		bool engineInitialized;
		bool soundInitialized;

		float* volume;
	public:
		explicit Player(FilesContainer& filesContainer, Saver& saver);
		~Player();

		static Player* getInstance();

		MusicFile* getCurFile();
		bool* getIsPlaying();

		void Play(MusicFile& file);
		void PlayNext();
		void PlayPrevious();
		void Pause();
		void Resume();

		const float& getVolume();
		void setVolume(float& volume);

		eventpp::CallbackList<void(MusicFile)> onMusicStarted;
		eventpp::CallbackList<void()> onMusicPaused;
		eventpp::CallbackList<void()> onMusicResumed;

		eventpp::CallbackList<void(float)> onVolumeChanged;

	private:
		void PlayAdjacent(int offset);
	};
}
