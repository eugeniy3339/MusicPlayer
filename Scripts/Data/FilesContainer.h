#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

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
		const std::string pathToMusicDirectory = "C:/Users/eugen/Music/TestMusic";

		static FilesContainer* instance;
		std::vector<MusicFile> files;
	public:
		FilesContainer();
		~FilesContainer();

		static FilesContainer* getInstance();
		void ReloadList();
		std::vector<MusicFile> GetFilesList(std::string path);
		std::vector<MusicFile>* getFiles();
	};
}