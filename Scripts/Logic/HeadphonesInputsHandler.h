#pragma once

#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <mutex>
#include <queue>

namespace MusicPlayer {
	class Player;

	class HeadphonesInputsHandler {
	public:
		explicit HeadphonesInputsHandler(Player& player);
		~HeadphonesInputsHandler();

		bool Initialize(HWND window);
		void ProcessPendingInputs();
		void Shutdown();

	private:
		enum class Input { Play, Pause, Next, Previous };
		void QueueInput(Input input);

		Player& player;
		winrt::Windows::Media::SystemMediaTransportControls controls{ nullptr };
		winrt::event_token buttonPressedToken{};
		std::mutex inputsMutex;
		std::queue<Input> pendingInputs;
	};
}
