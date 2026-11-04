#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Logic/Player.h"
#include <iostream>

using namespace MusicPlayer;

int main()
{
	FilesContainer filesContainer = FilesContainer::FilesContainer();
	Player player = Player();

	std::vector<MusicFile>* filesList = filesContainer.getFiles();
	if (filesList->empty()) {
		std::cerr << "No audio files found.\n";
		return 1;
	}

	player.Play((*filesList)[0]);

	std::cout << "Press Enter to exit.\n";
	std::cin.get();

	return 0;
}
