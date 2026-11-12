#include "../../external/nlohmann/json.hpp"
#include "Saver.h"
#include <fstream>
#include <string>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

namespace MusicPlayer {
	Saver::Saver() {
		jsonFilePath = GetJsonFile();
		std::ifstream in(jsonFilePath);
		if (in) {
			try {
				in >> curJson;
				if (!curJson.is_object())
					curJson = nlohmann::json::object();
			}
			catch (const nlohmann::json::exception&) {
				// Start with an empty settings object if the file is empty or invalid.
				curJson = nlohmann::json::object();
			}
		}
		else {
			WriteJson();
		}
	}
	Saver::~Saver() = default;

	std::filesystem::path Saver::GetJsonFile() {
		std::filesystem::path folder;
#ifdef _WIN32
		char* appData = nullptr;
		size_t size = 0;
		if (_dupenv_s(&appData, &size, "APPDATA") != 0 || appData == nullptr)
			throw std::runtime_error("APPDATA environment variable is unavailable");

		folder = std::filesystem::path(appData) / pathToJson;
		free(appData);
#else
		const char* configHome = std::getenv("XDG_CONFIG_HOME");
		if (configHome != nullptr && configHome[0] != '\0') {
			folder = std::filesystem::path(configHome) / pathToJson;
		} else {
			const char* home = std::getenv("HOME");
			if (home == nullptr || home[0] == '\0')
				throw std::runtime_error("HOME environment variable is unavailable");
			folder = std::filesystem::path(home) / ".config" / pathToJson;
		}
#endif
		std::filesystem::create_directories(folder);
		return folder / "Data.json";
	}

	void Saver::WriteJson() const {
		std::ofstream out(jsonFilePath);
		if (!out)
			throw std::runtime_error("Could not open settings file for writing: " + jsonFilePath.string());
		out << curJson.dump(4);
	}


	bool Saver::GetPath(std::string& path) {
		if (!curJson.contains("Path") || !curJson["Path"].is_string())
			return false;
		path = curJson["Path"].get<std::string>();
		return true;
	}
	void Saver::SavePath(std::string& path) {
		curJson["Path"] = path;
		WriteJson();
	}

	float Saver::GetVolume() {
		if (!curJson.contains("Volume") || !curJson["Volume"].is_number())
			return 1.0f;
		return curJson["Volume"].get<float>();
	}
	void Saver::SaveVolume(float& volume) {
		curJson["Volume"] = volume;
		WriteJson();
	}
}
