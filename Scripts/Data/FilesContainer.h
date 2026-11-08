#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Saver.h"
#include "../../external/eventpp/callbacklist.h"

namespace MusicPlayer {
	class MusicFile {
	private:
		std::filesystem::path path;
	public:
		MusicFile(std::filesystem::path path);
		~MusicFile();

		std::filesystem::path* getPath();
	};

	class FilesContainer {
	private:
		const std::string defaultPathToMusicDirectory = "C:\\Music";
		const std::string supportedExtensions[3]{ ".wav", ".mp3", ".flac" };
		static FilesContainer* instance;

		std::string pathToMusicDirectory;
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

		bool SupportsExtension(const std::string extension);

		eventpp::CallbackList<void()> onFilesListRefreshed;
	};
}
