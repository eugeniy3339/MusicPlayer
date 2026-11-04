#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Logic/Player.h"
#include "Scripts/Presentation/Visuals.h"
#include <iostream>

int main() {
	MusicPlayer::FilesContainer data{};
	MusicPlayer::Player player(data);
	MusicPlayer::Visuals visuals(player, data);
>>>>>>> Stashed changes
	return 0;
}
