#include "Commands.h"

namespace MusicPlayer {
	void PlayCommand::Start(std::string attribute) {
		auto files = FilesContainer::getInstance()->getFiles();
		auto file = files->at(0);
		Player::getInstance()->Play(file);
	}
}