#include "MusicPlayerView.h"

#include "../../external/imgui/imgui.h"
#include <algorithm>
#include <filesystem>
#include <shobjidl.h>
#include <thread>
#include <utility>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Uuid.lib")

namespace MusicPlayer {
	namespace {
		MusicFile* GetCurrentTrack(Player& player, FilesContainer& files) {
			MusicFile* current = player.getCurFile();
			if (current == nullptr)
				return nullptr;

			std::vector<MusicFile>* tracks = files.getFiles();
			const auto currentTrack = std::find_if(tracks->begin(), tracks->end(),
				[current](MusicFile& track) { return &track == current; });
			return currentTrack == tracks->end() ? nullptr : current;
		}

		std::wstring ChooseMusicFolder() {
			const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			if (FAILED(comResult))
				return {};

			std::wstring selectedPath;
			IFileOpenDialog* dialog = nullptr;
			HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
				CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
			if (SUCCEEDED(result)) {
				DWORD options = 0;
				result = dialog->GetOptions(&options);
				if (SUCCEEDED(result))
					result = dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
				if (SUCCEEDED(result))
					result = dialog->Show(nullptr);

				IShellItem* selectedItem = nullptr;
				if (SUCCEEDED(result))
					result = dialog->GetResult(&selectedItem);
				if (SUCCEEDED(result)) {
					PWSTR shellPath = nullptr;
					result = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &shellPath);
					if (SUCCEEDED(result) && shellPath != nullptr) {
						selectedPath = shellPath;
						CoTaskMemFree(shellPath);
					}
				}
				if (selectedItem != nullptr)
					selectedItem->Release();
				dialog->Release();
			}

			CoUninitialize();
			return selectedPath;
		}
	}

	MusicPlayerView::MusicPlayerView(Player& player, FilesContainer& files)
		: player(player), files(files), folderPickerState(std::make_shared<FolderPickerState>()) {
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

	void MusicPlayerView::StartFolderPicker() {
		const std::shared_ptr<FolderPickerState> state = folderPickerState;
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			if (state->running)
				return;
			state->running = true;
			state->completed = false;
			state->selectedPath.clear();
		}

		std::thread([state]() {
			std::wstring selectedPath = ChooseMusicFolder();
			std::lock_guard<std::mutex> lock(state->mutex);
			state->selectedPath = std::move(selectedPath);
			state->completed = true;
			state->running = false;
		}).detach();
	}

	void MusicPlayerView::Draw() {
		std::wstring selectedPath;
		{
			std::lock_guard<std::mutex> lock(folderPickerState->mutex);
			if (folderPickerState->completed) {
				selectedPath = std::move(folderPickerState->selectedPath);
				folderPickerState->completed = false;
			}
		}
		if (!selectedPath.empty())
			files.SetPathToMusicDirectory(std::filesystem::path(selectedPath).string());

		ImGui::SetNextWindowSize(ImVec2(900.0f, 620.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Music Player");

		ImGui::TextUnformatted("LIBRARY");
		ImGui::SameLine();
		ImGui::TextDisabled("%zu tracks", files.getFiles()->size());
		ImGui::SameLine();
		bool folderPickerRunning = false;
		{
			std::lock_guard<std::mutex> lock(folderPickerState->mutex);
			folderPickerRunning = folderPickerState->running;
		}
		if (folderPickerRunning)
			ImGui::BeginDisabled();
		if (ImGui::Button(folderPickerRunning ? "Choosing folder..." : "Choose folder"))
			StartFolderPicker();
		if (folderPickerRunning)
			ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
			files.RefreshList();
		ImGui::TextDisabled("%s", files.GetPathToMusicDirectory()->c_str());
		ImGui::Separator();

		const float footerHeight = 136.0f;
		if (ImGui::BeginChild("Library", ImVec2(0.0f, -footerHeight), ImGuiChildFlags_Borders)) {
			std::vector<MusicFile>* tracks = files.getFiles();
			MusicFile* current = GetCurrentTrack(player, files);
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
		MusicFile* current = GetCurrentTrack(player, files);
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
