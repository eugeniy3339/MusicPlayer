#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Saver.h"
#include "../../external/eventpp/callbacklist.h"

namespace MusicPlayer {
	class MusicFile {
	private:
		std::string path;
	public:
		MusicFile(std::string path);
		~MusicFile();

		std::string* getPath();
	};

	class FilesContainer {
	private:
		const std::string defaultPathToMusicDirectory = "C:\\Users\\eugen\\Music\\TestMusic";
		std::string pathToMusicDirectory;

		static FilesContainer* instance;
		std::vector<MusicFile> files;

		Saver& saver;
	public:
		FilesContainer(Saver& saver);
		~FilesContainer();

		static FilesContainer* getInstance();
		void RefreshList();
		void SetPathToMusicDirectory(std::string path);
		std::string* GetPathToMusicDirectory();
		std::vector<MusicFile> GetFilesList(std::string path);
		std::vector<MusicFile>* getFiles();

		eventpp::CallbackList<void()> onFilesListRefreshed;
	};
}
