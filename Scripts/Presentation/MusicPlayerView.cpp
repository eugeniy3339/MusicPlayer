#include "MusicPlayerView.h"

#include "../../external/imgui/imgui.h"
#include <algorithm>
#include <filesystem>

namespace MusicPlayer {
	MusicPlayerView::MusicPlayerView(Player& player, FilesContainer& files)
		: player(player), files(files) {
		// Immediate mode reads fresh state each frame; subscriptions keep the view
		// connected to the module lifecycle without copying or owning application data.
		filesChangedHandle = files.onFilesListRefreshed.append([]() {});
		musicStartedHandle = player.onMusicStarted.append([](MusicFile) {});
		musicPausedHandle = player.onMusicPaused.append([]() {});
		musicResumedHandle = player.onMusicResumed.append([]() {});
	}

	MusicPlayerView::~MusicPlayerView() {
		files.onFilesListRefreshed.remove(filesChangedHandle);
		player.onMusicStarted.remove(musicStartedHandle);
		player.onMusicPaused.remove(musicPausedHandle);
		player.onMusicResumed.remove(musicResumedHandle);
	}

	void MusicPlayerView::Draw() {
		ImGui::SetNextWindowSize(ImVec2(900.0f, 620.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Music Player");

		ImGui::TextUnformatted("LIBRARY");
		ImGui::SameLine();
		ImGui::TextDisabled("%zu tracks", files.getFiles()->size());
		ImGui::Separator();

		const float footerHeight = 136.0f;
		if (ImGui::BeginChild("Library", ImVec2(0.0f, -footerHeight), ImGuiChildFlags_Borders)) {
			std::vector<MusicFile>* tracks = files.getFiles();
			MusicFile* current = player.getCurFile();
			for (std::size_t index = 0; index < tracks->size(); ++index) {
				MusicFile& track = (*tracks)[index];
				const std::string& path = *track.getPath();
				const std::string title = std::filesystem::path(path).stem().string();
				const bool isCurrent = current == &track;
				ImGui::PushID(static_cast<int>(index));
				if (ImGui::Selectable(title.c_str(), isCurrent, ImGuiSelectableFlags_AllowDoubleClick))
					player.Play(track);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", path.c_str());
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		ImGui::Separator();
		MusicFile* current = player.getCurFile();
		const bool playing = *player.getIsPlaying();
		if (current != nullptr) {
			const std::string title = std::filesystem::path(*current->getPath()).stem().string();
			ImGui::TextUnformatted(title.c_str());
			ImGui::TextDisabled("%s", current->getPath()->c_str());
		} else {
			ImGui::TextUnformatted("Nothing playing");
			ImGui::TextDisabled("Choose a track from your library");
		}

		const float buttonWidth = 76.0f;
		const float controlsWidth = buttonWidth * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - controlsWidth) * 0.5f);
		if (ImGui::Button("Previous", ImVec2(buttonWidth, 30.0f)))
			player.PlayPrevious();
		ImGui::SameLine();
		if (ImGui::Button(playing ? "Pause" : "Play", ImVec2(buttonWidth, 30.0f))) {
			if (playing)
				player.Pause();
			else if (current != nullptr)
				player.Resume();
			else
				player.PlayNext();
		}
		ImGui::SameLine();
		if (ImGui::Button("Next", ImVec2(buttonWidth, 30.0f)))
			player.PlayNext();

		ImGui::End();
	}
}
