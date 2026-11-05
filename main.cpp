#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Data/Saver.h"
#include "Scripts/Logic/Player.h"


int main() {
	MusicPlayer::Saver saver{};
	MusicPlayer::FilesContainer data(saver);
	MusicPlayer::Player player(data);
	return 0;
}
