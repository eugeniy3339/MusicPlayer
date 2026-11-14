#include "HeadphonesInputsHandler.h"

#include "Player.h"
#include <SystemMediaTransportControlsInterop.h>
#include <winrt/base.h>

#pragma comment(lib, "runtimeobject.lib")

namespace MusicPlayer {
	using namespace winrt::Windows::Media;

	HeadphonesInputsHandler::HeadphonesInputsHandler(Player& player) : player(player) {}

	HeadphonesInputsHandler::~HeadphonesInputsHandler() {
		Shutdown();
	}

	bool HeadphonesInputsHandler::Initialize(HWND window) {
		try {
			auto interop = winrt::get_activation_factory<SystemMediaTransportControls,
				ISystemMediaTransportControlsInterop>();
			winrt::check_hresult(interop->GetForWindow(window,
				winrt::guid_of<SystemMediaTransportControls>(), winrt::put_abi(controls)));

			controls.IsEnabled(true);
			controls.IsPlayEnabled(true);
			controls.IsPauseEnabled(true);
			controls.IsNextEnabled(true);
			controls.IsPreviousEnabled(true);
			controls.PlaybackStatus(MediaPlaybackStatus::Paused);
			buttonPressedToken = controls.ButtonPressed([this](const auto&, const auto& args) {
				switch (args.Button()) {
				case SystemMediaTransportControlsButton::Play: QueueInput(Input::Play); break;
				case SystemMediaTransportControlsButton::Pause: QueueInput(Input::Pause); break;
				case SystemMediaTransportControlsButton::Next: QueueInput(Input::Next); break;
				case SystemMediaTransportControlsButton::Previous: QueueInput(Input::Previous); break;
				}
			});
			return true;
		}
		catch (const winrt::hresult_error&) {
			Shutdown();
			return false;
		}
	}

	void HeadphonesInputsHandler::QueueInput(Input input) {
		std::lock_guard<std::mutex> lock(inputsMutex);
		pendingInputs.push(input);
	}

	void HeadphonesInputsHandler::ProcessPendingInputs() {
		std::queue<Input> inputs;
		{
			std::lock_guard<std::mutex> lock(inputsMutex);
			inputs.swap(pendingInputs);
		}
		while (!inputs.empty()) {
			switch (inputs.front()) {
			case Input::Play:
				player.Resume();
				if (controls) controls.PlaybackStatus(MediaPlaybackStatus::Playing);
				break;
			case Input::Pause:
				player.Pause();
				if (controls) controls.PlaybackStatus(MediaPlaybackStatus::Paused);
				break;
			case Input::Next:
				player.PlayNext();
				if (controls) controls.PlaybackStatus(MediaPlaybackStatus::Playing);
				break;
			case Input::Previous:
				player.PlayPrevious();
				if (controls) controls.PlaybackStatus(MediaPlaybackStatus::Playing);
				break;
			}
			inputs.pop();
		}
	}

	void HeadphonesInputsHandler::Shutdown() {
		if (controls) {
			controls.ButtonPressed(buttonPressedToken);
			controls.IsEnabled(false);
			controls = nullptr;
			buttonPressedToken = {};
		}
	}
}
