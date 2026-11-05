#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Data/Saver.h"
#include "Scripts/Logic/Player.h"
#include "Scripts/Presentation/Visuals.h"


int main() {
	MusicPlayer::Saver saver{};
	MusicPlayer::FilesContainer data(saver);
	MusicPlayer::Player player(data);
	MusicPlayer::Visuals visuals(player, data);
	return 0;
}
