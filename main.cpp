#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Data/Saver.h"
#include "Scripts/Logic/Player.h"

#include <exception>
#include <iostream>

int main() {
	MusicPlayer::Saver saver{};
	MusicPlayer::FilesContainer data(saver);
	MusicPlayer::Player player(data, saver);

	while (true) {
		player.Update();
	}

	return 0;
}
