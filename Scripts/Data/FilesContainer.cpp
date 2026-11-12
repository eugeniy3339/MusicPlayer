#include "FilesContainer.h"
#include <string>
#include <vector>
#include <filesystem>

namespace MusicPlayer {
	namespace {
		std::string PathToUtf8(const std::filesystem::path& path) {
			auto value = path.u8string();
			return std::string(value.begin(), value.end());
		}
	}
#pragma region  MusicFile

	std::filesystem::path* MusicFile::getPath() {
		return &path;
	}


	std::string MusicFile::getName(const std::filesystem::path& filePath) {
		return PathToUtf8(filePath.stem());
	}

	std::string* MusicFile::getName() {
		return &name;
	}

	MusicFile::MusicFile(std::filesystem::path path) {
		this->path = path;
		name = getName(this->path);
	}
	MusicFile::~MusicFile() = default;

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
		const auto root = std::filesystem::path(std::u8string(path.begin(), path.end()));
		if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
			return filesList;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
			if (entry.is_regular_file() && SupportsExtension(PathToUtf8(entry.path().extension()))) {
				MusicFile file(entry.path());
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
		saver.SavePath(pathToMusicDirectory);
		RefreshList();
	}

	std::string* FilesContainer::GetPathToMusicDirectory() {
		return &pathToMusicDirectory;
	}

	bool FilesContainer::SupportsExtension(const std::string extension) {
		for (auto ext : supportedExtensions)
		{
			if (ext == extension)
				return true;
		}

		return false;
	}
#pragma endregion
}

