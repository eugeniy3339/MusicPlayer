#include "FilesContainer.h"
#include "Player.h"
#include "../external/miniaudio.h"

namespace MusicPlayer
{
	Player* Player::instance = nullptr;

	Player::Player() {
		if (instance != nullptr)
		{
			delete this;
			return;
		}
		instance = this;
		isPlaying = false;
		curFile = nullptr;
	}
	Player::~Player() {
	}

	Player* Player::getInstance() {
		return instance;
	}

	MusicFile* Player::getCurFile() {
		return curFile;
	}
	bool* Player::getIsPlaying() {
		return &isPlaying;
	}

	void Player::Play(MusicFile& file) {
		curFile = &file;
		isPlaying = true;
	}

	void Player::Pause() {
		isPlaying = false;
	}

	void Player::Resume() {
		isPlaying = true;
	}
}