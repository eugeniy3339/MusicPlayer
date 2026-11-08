#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Data/Saver.h"
#include "Scripts/Logic/Player.h"
#include "Scripts/Presentation/Visuals.h"

#include <exception>
#include <iostream>

int main() {
	MusicPlayer::Saver saver{};
	MusicPlayer::FilesContainer data(saver);
	MusicPlayer::Player player(data, saver);
	MusicPlayer::Visuals visuals(player, data);

	while (true) {
		if (!visuals.Update())
			break;
		player.Update();
	}

	return 0;
}
