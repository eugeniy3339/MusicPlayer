#include "MusicPlayerView.h"

#include "../../external/imgui/imgui.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace MusicPlayer {
	MusicPlayerView::MusicPlayerView() : selectedIndex(0) {
		std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", "C:\\Users\\eugen\\Music\\TestMusic");
	}

	void MusicPlayerView::ChangePath(FilesContainer& filesContainer, Player& player) {
		if (*player.getIsPlaying())
			player.Pause();
		filesContainer.SetPathToMusicDirectory(pathBuffer);
		selectedIndex = 0;
	}

	void MusicPlayerView::PlaySelected(FilesContainer& filesContainer, Player& player) {
		std::vector<MusicFile>* files = filesContainer.getFiles();
		if (selectedIndex >= 0 && selectedIndex < static_cast<int>(files->size()))
			player.Play((*files)[selectedIndex]);
	}

	void MusicPlayerView::SyncSelectionWithPlayer(FilesContainer& filesContainer, Player& player) {
		std::vector<MusicFile>* files = filesContainer.getFiles();
		MusicFile* currentFile = player.getCurFile();
		for (int index = 0; index < static_cast<int>(files->size()); ++index) {
			if (&(*files)[index] == currentFile) {
				selectedIndex = index;
				return;
			}
		}
	}

	void MusicPlayerView::Render(FilesContainer& filesContainer, Player& player) {
		ImGui::SetNextWindowSize(ImVec2(520.0f, 560.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Music Player")) {
			ImGui::End();
			return;
		}

		ImGui::TextUnformatted("Music folder");
		ImGui::SetNextItemWidth(-1.0f);
		const bool pathSubmitted = ImGui::InputText(
			"##music-folder", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
		if (ImGui::Button("Change Path") || pathSubmitted)
			ChangePath(filesContainer, player);

		std::vector<MusicFile>* files = filesContainer.getFiles();
		ImGui::Separator();
		ImGui::Text("Available sounds (%d)", static_cast<int>(files->size()));

		const float listHeight = std::max(120.0f, ImGui::GetContentRegionAvail().y - 92.0f);
		if (ImGui::BeginChild("##sound-list", ImVec2(0.0f, listHeight), ImGuiChildFlags_Borders)) {
			for (int index = 0; index < static_cast<int>(files->size()); ++index) {
				const std::string name = std::filesystem::path(*(*files)[index].getPath()).filename().string();
				if (ImGui::Selectable(name.c_str(), selectedIndex == index))
					selectedIndex = index;
			}
		}
		ImGui::EndChild();

		MusicFile* currentFile = player.getCurFile();
		const bool selectedIsCurrent = selectedIndex >= 0 &&
			selectedIndex < static_cast<int>(files->size()) &&
			currentFile == &(*files)[selectedIndex];

		ImGui::BeginDisabled(files->empty());
		if (ImGui::Button("Previous")) {
			player.PlayPrevious();
			SyncSelectionWithPlayer(filesContainer, player);
		}
		ImGui::SameLine();
		if (ImGui::Button("Play", ImVec2(100.0f, 0.0f)))
			PlaySelected(filesContainer, player);
		ImGui::SameLine();
		const char* playbackLabel = *player.getIsPlaying()
			? "Pause"
			: (selectedIsCurrent ? "Resume" : "Play");
		if (ImGui::Button(playbackLabel)) {
			if (*player.getIsPlaying())
				player.Pause();
			else if (selectedIsCurrent)
				player.Resume();
			else
				PlaySelected(filesContainer, player);
		}
		ImGui::SameLine();
		if (ImGui::Button("Next")) {
			player.PlayNext();
			SyncSelectionWithPlayer(filesContainer, player);
		}
		ImGui::EndDisabled();

		if (files->empty())
			ImGui::TextDisabled("No files found in this folder.");

		ImGui::End();
	}
}
