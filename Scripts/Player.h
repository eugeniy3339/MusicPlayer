#pragma once
#include "FilesContainer.h"
#include "../external/miniaudio.h"

namespace MusicPlayer {
	class Player {
	private:
		static Player* instance;

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
		void Pause();
		void Resume();

		Player();
		~Player();
	};
}