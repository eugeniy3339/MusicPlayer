#define MINIAUDIO_IMPLEMENTATION
#include "Player.h"
#include "../Data/FilesContainer.h"
#include "../../external/miniaudio.h"
#include <iostream>

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
		engineInitialized = ma_engine_init(nullptr, &engine) == MA_SUCCESS;
		soundInitialized = false;
		if (!engineInitialized)
			std::cerr << "Failed to initialize miniaudio engine.\n";
	}
	Player::~Player() {
		if (soundInitialized)
			ma_sound_uninit(&sound);
		if (engineInitialized)
			ma_engine_uninit(&engine);
		if (instance == this)
			instance = nullptr;
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
		if (!engineInitialized)
			return;

		if (soundInitialized) {
			ma_sound_uninit(&sound);
			soundInitialized = false;
		}
		isPlaying = false;
		curFile = nullptr;

		const ma_result result = ma_sound_init_from_file(
			&engine, file.getPath()->c_str(), 0, nullptr, nullptr, &sound);
		if (result != MA_SUCCESS) {
			std::cerr << "Failed to open audio file: " << *file.getPath()
				<< " (miniaudio error " << result << ").\n";
			return;
		}

		soundInitialized = true;
		curFile = &file;
		if (ma_sound_start(&sound) == MA_SUCCESS)
		{
			isPlaying = true;
			std::cout << "Playing!!!!!\n";
		}
		else
			std::cerr << "Failed to start audio file: " << *file.getPath() << '\n';
	}

	void Player::Pause() {
		if (soundInitialized && isPlaying && ma_sound_stop(&sound) == MA_SUCCESS)
			isPlaying = false;
	}

	void Player::Resume() {
		if (soundInitialized && !isPlaying && ma_sound_start(&sound) == MA_SUCCESS)
			isPlaying = true;
	}
}
