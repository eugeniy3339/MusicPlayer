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
		std::string name;

		std::string getName(const std::filesystem::path& filePath);
	public:
		MusicFile(std::filesystem::path path);
		~MusicFile();

		std::filesystem::path* getPath();
		std::string* getName();
	};

	class FilesContainer {
	private:
#ifdef _WIN32
		const std::string defaultPathToMusicDirectory = "C:\\Music";
#else
		const std::string defaultPathToMusicDirectory = "~/Music";
#endif
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
