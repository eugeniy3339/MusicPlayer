#include "Scripts/FilesContainer.h"
#include "Scripts/Player.h"

using namespace MusicPlayer;

int main()
{
	FilesContainer filesContainer = FilesContainer::FilesContainer();
	Player player = Player();

	std::vector<MusicFile>* filesList = filesContainer.getFiles();
	player.Play((*filesList)[0]);

	return 0;
}