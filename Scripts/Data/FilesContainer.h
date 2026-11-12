#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "../musicplayerfunctions.h"
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

		std::vector<MusicFile> GetMatchingFiles(const std::wstring search) {
			std::vector<MusicFile> matches;
			auto* fileList = getFiles();
			std::wstring lowerSearch = Lowercase(search);
			for (auto file : *fileList) {
				const std::wstring name = Lowercase(Utf8ToWide(*file.getName()));
				if (name.find(lowerSearch) != std::wstring::npos)
					matches.push_back(file);
			}
			return matches;
		}

		eventpp::CallbackList<void()> onFilesListRefreshed;
	};
}
