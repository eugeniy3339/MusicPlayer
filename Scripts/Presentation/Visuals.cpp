#include "Visuals.h"

#include <conio.h>
#include <windows.h>

#include <filesystem>
#include <utility>
#include <vector>

namespace MusicPlayer {
	namespace {
		std::wstring Utf8ToWide(const std::string& value) {
			if (value.empty())
				return {};

			const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(),
				static_cast<int>(value.size()), nullptr, 0);
			if (size <= 0)
				return std::wstring(value.begin(), value.end());

			std::wstring result(size, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
				result.data(), size);
			return result;
		}

		std::wstring Lowercase(std::wstring value) {
			if (value.empty())
				return value;
			const int size = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
				value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr, 0);
			if (size <= 0)
				return value;
			std::wstring result(size, L'\0');
			LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, value.data(),
				static_cast<int>(value.size()), result.data(), size, nullptr, nullptr, 0);
			return result;
		}
	}

	Visuals::Visuals(Player& player, FilesContainer& files) : player(player), files(files) {
		player.onMusicStarted.append([this](MusicFile file) {
			currentTrackPath = file.getPath()->wstring();
			isDirty = true;
		});
		player.onMusicPaused.append([this]() { isDirty = true; });
		player.onMusicResumed.append([this]() { isDirty = true; });
		player.onVolumeChanged.append([this](float) { isDirty = true; });
		files.onFilesListRefreshed.append([this]() { isDirty = true; });
	}

	bool Visuals::Update() {
		while (_kbhit()) {
			int key = _getwch();
			if (key == 0 || key == 0xE0) {
				key = _getwch();
				if (!enteringPath && !enteringSearch && (key == 72 || key == 80))
					HandleKey(key == 72 ? 'w' : 's');
				continue;
			}
			HandleKey(key);
		}

		CONSOLE_SCREEN_BUFFER_INFO screenInfo{};
		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(console, &screenInfo)) {
			const int width = screenInfo.srWindow.Right - screenInfo.srWindow.Left + 1;
			const int height = screenInfo.srWindow.Bottom - screenInfo.srWindow.Top + 1;
			if (width != lastConsoleWidth || height != lastConsoleHeight) {
				lastConsoleWidth = width;
				lastConsoleHeight = height;
				isDirty = true;
			}
		}

		if (isDirty) {
			Render();
			isDirty = false;
		}
		Sleep(16);
		return isRunning;
	}

	void Visuals::HandleKey(int key) {
		isDirty = true;
		if (enteringSearch) {
			if (key == '\r' || key == '\n') {
				enteringSearch = false;
				selectedFile = 0;
				statusMessage = searchInput.empty() ? "Search cleared." : "Search applied.";
			}
			else if (key == 27) {
				enteringSearch = false;
				searchInput.clear();
				selectedFile = 0;
				statusMessage = "Search cleared.";
			}
			else if (key == '\b' || key == 127) {
				if (!searchInput.empty()) {
					searchInput.pop_back();
					if (!searchInput.empty() && searchInput.back() >= 0xD800 && searchInput.back() <= 0xDBFF)
						searchInput.pop_back();
				}
				selectedFile = 0;
			}
			else if (key >= 32 && key <= 0xFFFF) {
				searchInput.push_back(static_cast<wchar_t>(key));
				selectedFile = 0;
			}
			return;
		}
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

		auto matches = GetMatchingFiles();
		switch (key) {
		case 'w': case 'W':
			if (!matches.empty() && selectedFile > 0)
				--selectedFile;
			break;
		case 's': case 'S':
			if (!matches.empty() && selectedFile + 1 < matches.size())
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
			matches = GetMatchingFiles();
			if (matches.empty())
				selectedFile = 0;
			else if (selectedFile >= matches.size())
				selectedFile = matches.size() - 1;
			statusMessage = "File list refreshed.";
			break;
		case 'c': case 'C':
			enteringPath = true;
			pathInput.clear();
			statusMessage = "Type a directory path, then press Enter (Esc cancels).";
			break;
		case '/':
			enteringSearch = true;
			statusMessage = "Type a name search, then press Enter (Esc clears).";
			break;
		case 'q': case 'Q':
			isRunning = false;
			break;
		default:
			break;
		}
	}

	std::vector<std::size_t> Visuals::GetMatchingFiles() {
		std::vector<std::size_t> matches;
		auto* fileList = files.getFiles();
		const std::wstring needle = Lowercase(searchInput);
		for (std::size_t i = 0; i < fileList->size(); ++i) {
			const std::wstring name = Lowercase(Utf8ToWide(*(*fileList)[i].getName()));
			if (name.find(needle) != std::wstring::npos)
				matches.push_back(i);
		}
		return matches;
	}

	void Visuals::PlaySelected() {
		auto* fileList = files.getFiles();
		auto matches = GetMatchingFiles();
		if (matches.empty() || selectedFile >= matches.size()) {
			statusMessage = "No sound files are available in this directory.";
			return;
		}
		player.Play((*fileList)[matches[selectedFile]]);
	}

	void Visuals::Render() {
		const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO screenInfo{};
		if (console == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(console, &screenInfo))
			return;

		const int width = screenInfo.srWindow.Right - screenInfo.srWindow.Left + 1;
		const int height = screenInfo.srWindow.Bottom - screenInfo.srWindow.Top + 1;
		if (width <= 0 || height <= 0)
			return;

		auto* fileList = files.getFiles();
		auto matches = GetMatchingFiles();
		const bool isPlaying = *player.getIsPlaying();
		const float volume = player.getVolume();
		const int filled = static_cast<int>(volume * 20.0f + 0.5f);
		std::vector<std::wstring> lines;
		const auto addLine = [&lines, width](std::wstring line) {
			if (line.size() >= static_cast<std::size_t>(width))
				line.resize(static_cast<std::size_t>(width - 1));
			lines.push_back(std::move(line));
		};

		addLine(L"MUSIC PLAYER");
		addLine(L"============");
		addLine(L"");
		addLine(L"Sounds in: " + Utf8ToWide(*files.GetPathToMusicDirectory()));
		addLine(L"Search: " + searchInput + (enteringSearch ? L"_" : L""));

		const int listCapacity = height > 12 ? height - 12 : 1;
		const std::size_t visibleCount = static_cast<std::size_t>(listCapacity);
		const std::size_t firstVisible = selectedFile >= visibleCount
			? selectedFile - visibleCount + 1
			: 0;
		if (matches.empty())
			addLine(L"  (no supported sound files found)");
		else {
			for (std::size_t row = 0; row < visibleCount; ++row) {
				const std::size_t index = firstVisible + row;
				if (index >= matches.size())
					break;
				const std::wstring name = Utf8ToWide(*(*fileList)[matches[index]].getName());
				addLine((index == selectedFile ? L" > " : L"   ") + name);
			}
		}

		while (lines.size() < static_cast<std::size_t>(4 + listCapacity))
			addLine(L"");

		addLine(L"");
		addLine(std::wstring(isPlaying ? L"Now playing: " : L"Now paused: ") +
			(currentTrackPath.empty() ? L"(nothing selected)" : currentTrackPath));

		std::wstring volumeBar = L"Volume [";
		for (int i = 0; i < 20; ++i)
			volumeBar += i < filled ? L'#' : L'-';
		volumeBar += L"] " + std::to_wstring(static_cast<int>(volume * 100.0f + 0.5f)) + L"%";
		addLine(std::move(volumeBar));

		if (enteringSearch)
			addLine(L"Search: type name, Enter apply, Esc clear.");
		else if (enteringPath)
			addLine(L"New music directory: " + Utf8ToWide(pathInput) + L"_");
		else if (!statusMessage.empty())
			addLine(Utf8ToWide(statusMessage));
		else
			addLine(L"");

		addLine(L"Controls: W/S select  Enter play  Space pause/resume  N next  P previous");
		addLine(L"          +/- volume  C change directory  R refresh files  Q quit");
		addLine(L"          / search by track name");

		const std::size_t cellCount = static_cast<std::size_t>(width) * height;
		std::vector<CHAR_INFO> buffer(cellCount);
		const std::wstring emptyLine;
		for (std::size_t row = 0; row < static_cast<std::size_t>(height); ++row) {
			const std::wstring& line = row < lines.size() ? lines[row] : emptyLine;
			for (int column = 0; column < width; ++column) {
				CHAR_INFO& cell = buffer[row * width + column];
				cell.Char.UnicodeChar = static_cast<std::size_t>(column) < line.size()
					? line[column]
					: L' ';
				cell.Attributes = screenInfo.wAttributes;
			}
		}

		const COORD bufferSize{ static_cast<SHORT>(width), static_cast<SHORT>(height) };
		const COORD bufferOrigin{ 0, 0 };
		SMALL_RECT target = screenInfo.srWindow;
		WriteConsoleOutputW(console, buffer.data(), bufferSize, bufferOrigin, &target);
	}
}
