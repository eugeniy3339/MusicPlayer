#pragma once
#include"../../external/nlohmann/json.hpp"
#include <filesystem>
#include<string>

namespace MusicPlayer {

	class Saver {
	private:
		const std::string pathToJson = "eugeniy3339/MusicPlayer/";

		nlohmann::json curJson = nlohmann::json::object();
		std::filesystem::path jsonFilePath;

		std::filesystem::path GetJsonFile();
		void WriteJson() const;
	public:
		bool GetPath(std::string& path);
		void SavePath(std::string* path);

		Saver();
		~Saver();
	};
}
