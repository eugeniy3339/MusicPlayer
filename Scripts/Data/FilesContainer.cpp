#include "FilesContainer.h"
#include <string>
#include <vector>
#include <filesystem>

namespace MusicPlayer {
#pragma region  MusicFile

	std::string* MusicFile::getPath() {
		return &path;
	}

	MusicFile::MusicFile(std::string path) {
		this->path = path;
	}
	MusicFile::~MusicFile() {
	}

#pragma endregion
#pragma region FilesContainer

	FilesContainer* FilesContainer::instance = nullptr;
	
	FilesContainer::FilesContainer(Saver& saver) : saver(saver) {
		if (instance != nullptr)
		{
			delete this;
			return;
		}
		instance = this;
		std::string path;
		if (this->saver.GetPath(path))
		{
			SetPathToMusicDirectory(path);
		}
		else
		{
			SetPathToMusicDirectory(defaultPathToMusicDirectory);
		}
		files = GetFilesList(pathToMusicDirectory);
	}

	FilesContainer::~FilesContainer() {
	}



	FilesContainer* FilesContainer::getInstance() {
		return instance;
	}

	void FilesContainer::RefreshList() {
		files = GetFilesList(pathToMusicDirectory);
		onFilesListRefreshed();
	}

	std::vector<MusicFile> FilesContainer::GetFilesList(std::string path) {
		std::vector<MusicFile> filesList;
		if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
			return filesList;
		}

		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (entry.is_regular_file()) {
				MusicFile file(entry.path().string());
				filesList.push_back(file);
			}
		}

		return filesList;
	}

	std::vector<MusicFile>* FilesContainer::getFiles() {
		return &files;
	}

	void FilesContainer::SetPathToMusicDirectory(std::string path) {
		pathToMusicDirectory = path;
		saver.SavePath(&pathToMusicDirectory);
		RefreshList();
	}

	std::string* FilesContainer::GetPathToMusicDirectory() {
		return &pathToMusicDirectory;
	}
#pragma endregion
}

