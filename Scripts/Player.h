#pragma once
#include "FilesContainer.h"

namespace MusicPlayer {
	class Player {
	private:
		static Player* instance;

		MusicFile* curFile;
		bool isPlaying;
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