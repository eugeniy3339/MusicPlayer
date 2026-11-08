#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Data/Saver.h"
#include "Scripts/Logic/Player.h"
#include "Scripts/CommandsListener.h"
//#include "Scripts/Presentation/Visuals.h"


int main() {
	MusicPlayer::Saver saver{};
	MusicPlayer::FilesContainer data(saver);
	MusicPlayer::Player player(data, saver);
	MusicPlayer::CommandsListener commandsListener;
	//MusicPlayer::Visuals visuals(player, data);
	while (true) {
		player.Update();
		commandsListener.Update();
		/*if (!visuals.Update())
			break;*/
	}
	return 0;
}
