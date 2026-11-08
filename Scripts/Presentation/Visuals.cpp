#include "Visuals.h"

#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace MusicPlayer {
	namespace {
		std::string PathToUtf8(const std::filesystem::path& path) {
			const auto utf8Path = path.u8string();
			return std::string(utf8Path.begin(), utf8Path.end());
		}
	}

	Visuals::Visuals(Player& player, FilesContainer& files) : player(player), files(files) {
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD consoleMode = 0;
		if (console != INVALID_HANDLE_VALUE && GetConsoleMode(console, &consoleMode))
			SetConsoleMode(console, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

		player.onMusicStarted.append([this](MusicFile file) {
			currentTrackPath = PathToUtf8(*file.getPath());
		});
	}

	bool Visuals::Update() {
		while (_kbhit()) {
			int key = _getch();
			if (key == 0 || key == 0xE0) {
				key = _getch();
				if (!enteringPath && (key == 72 || key == 80))
					HandleKey(key == 72 ? 'w' : 's');
				continue;
			}
			HandleKey(key);
		}

		const auto now = std::chrono::steady_clock::now();
		if (lastRender.time_since_epoch().count() == 0 || now - lastRender >= std::chrono::milliseconds(200)) {
			Render();
			lastRender = now;
		}
		Sleep(16);
		return isRunning;
	}

	void Visuals::HandleKey(int key) {
		if (enteringPath) {
			if (key == '\r' || key == '\n') {
				enteringPath = false;
				if (!pathInput.empty()) {
					files.SetPathToMusicDirectory(pathInput);
					selectedFile = 0;
					statusMessage = "Music directory updated.";
				}
				else {
					statusMessage = "Path change cancelled.";
				}
				pathInput.clear();
			}
			else if (key == 27) {
				enteringPath = false;
				pathInput.clear();
				statusMessage = "Path change cancelled.";
			}
			else if (key == '\b' || key == 127) {
				if (!pathInput.empty())
					pathInput.pop_back();
			}
			else if (key >= 32 && key <= 126) {
				pathInput.push_back(static_cast<char>(key));
			}
			return;
		}

		auto* fileList = files.getFiles();
		switch (key) {
		case 'w': case 'W':
			if (!fileList->empty() && selectedFile > 0)
				--selectedFile;
			break;
		case 's': case 'S':
			if (!fileList->empty() && selectedFile + 1 < fileList->size())
				++selectedFile;
			break;
		case '\r': case '\n':
			PlaySelected();
			break;
		case ' ':
			if (*player.getIsPlaying())
				player.Pause();
			else
				player.Resume();
			break;
		case 'n': case 'N':
			player.PlayNext();
			break;
		case 'p': case 'P':
			player.PlayPrevious();
			break;
		case '+': case '=':
			player.setVolume(player.getVolume() + 0.05f);
			break;
		case '-': case '_':
			player.setVolume(player.getVolume() - 0.05f);
			break;
		case 'r': case 'R':
			files.RefreshList();
			if (fileList->empty())
				selectedFile = 0;
			else if (selectedFile >= fileList->size())
				selectedFile = fileList->size() - 1;
			statusMessage = "File list refreshed.";
			break;
		case 'c': case 'C':
			enteringPath = true;
			pathInput.clear();
			statusMessage = "Type a directory path, then press Enter (Esc cancels).";
			break;
		case 'q': case 'Q':
			isRunning = false;
			break;
		default:
			break;
		}
	}

	void Visuals::PlaySelected() {
		auto* fileList = files.getFiles();
		if (fileList->empty() || selectedFile >= fileList->size()) {
			statusMessage = "No sound files are available in this directory.";
			return;
		}
		player.Play((*fileList)[selectedFile]);
	}

	void Visuals::Render() {
		auto* fileList = files.getFiles();
		const bool isPlaying = *player.getIsPlaying();
		const float volume = player.getVolume();
		const int filled = static_cast<int>(volume * 20.0f + 0.5f);

		std::cout << "\x1b[2J\x1b[H";
		std::cout << "MUSIC PLAYER\n"
			<< "============\n\n"
			<< "Sounds in: " << *files.GetPathToMusicDirectory() << "\n";

		if (fileList->empty()) {
			std::cout << "  (no supported sound files found)\n";
		}
		else {
			for (std::size_t i = 0; i < fileList->size(); ++i) {
				const std::string name = PathToUtf8((*fileList)[i].getPath()->filename());
				std::cout << (i == selectedFile ? " > " : "   ") << name << '\n';
			}
		}

		std::cout << "\nNow " << (isPlaying ? "playing: " : "paused: ");
		if (currentTrackPath.empty())
			std::cout << "(nothing selected)\n";
		else
			std::cout << currentTrackPath << '\n';

		std::cout << "\nVolume [";
		for (int i = 0; i < 20; ++i)
			std::cout << (i < filled ? '#' : '-');
		std::cout << "] " << std::setw(3) << static_cast<int>(volume * 100.0f + 0.5f) << "%\n\n";

		if (enteringPath)
			std::cout << "New music directory: " << pathInput << "_\n\n";
		else if (!statusMessage.empty())
			std::cout << statusMessage << "\n\n";

		std::cout << "Controls: W/S select  Enter play  Space pause/resume  N next  P previous\n"
			<< "          +/- volume  C change directory  R refresh files  Q quit\n"
			<< std::flush;
	}
}
