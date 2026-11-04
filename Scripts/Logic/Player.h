#pragma once
#include "../Data/FilesContainer.h"
#include "../../external/miniaudio.h"

namespace MusicPlayer {
	class Player {
	private:
		static Player* instance;
		FilesContainer& filesContainer;

		MusicFile* curFile;
		bool isPlaying;
		ma_engine engine;
		ma_sound sound;
		bool engineInitialized;
		bool soundInitialized;
	public:
		static Player* getInstance();

		MusicFile* getCurFile();
		bool* getIsPlaying();

		void Play(MusicFile& file);
		void PlayNext();
		void PlayPrevious();
		void Pause();
		void Resume();

		explicit Player(FilesContainer& filesContainer);
		~Player();

	private:
		void PlayAdjacent(int offset);
	};
}
