#include "Visuals.h"

#include "../../external/imgui/backends/imgui_impl_dx11.h"
#include "../../external/imgui/backends/imgui_impl_win32.h"
#include "../../external/imgui/imgui.h"

#include <d3d11.h>
#include <windows.h>
#include <filesystem>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace MusicPlayer {
	namespace {
		ID3D11Device* g_device = nullptr;
		ID3D11DeviceContext* g_context = nullptr;
		IDXGISwapChain* g_swapChain = nullptr;
		ID3D11RenderTargetView* g_renderTarget = nullptr;
		UINT g_resizeWidth = 0;
		UINT g_resizeHeight = 0;

		void CreateRenderTarget() {
			ID3D11Texture2D* backBuffer = nullptr;
			if (g_swapChain && SUCCEEDED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
				g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
				backBuffer->Release();
			}
		}

		void CleanupRenderTarget() {
			if (g_renderTarget) {
				g_renderTarget->Release();
				g_renderTarget = nullptr;
			}
		}

		bool CreateDeviceD3D(HWND window) {
			DXGI_SWAP_CHAIN_DESC desc{};
			desc.BufferCount = 2;
			desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BufferDesc.RefreshRate.Numerator = 60;
			desc.BufferDesc.RefreshRate.Denominator = 1;
			desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.OutputWindow = window;
			desc.SampleDesc.Count = 1;
			desc.Windowed = TRUE;
			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
			D3D_FEATURE_LEVEL level{};
			if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
				levels, 2, D3D11_SDK_VERSION, &desc, &g_swapChain, &g_device, &level, &g_context)))
				return false;
			CreateRenderTarget();
			return g_renderTarget != nullptr;
		}

		void CleanupDeviceD3D() {
			CleanupRenderTarget();
			if (g_swapChain) { g_swapChain->Release(); g_swapChain = nullptr; }
			if (g_context) { g_context->Release(); g_context = nullptr; }
			if (g_device) { g_device->Release(); g_device = nullptr; }
		}

		LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
			if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam))
				return 1;
			switch (message) {
			case WM_SIZE:
				if (wParam != SIZE_MINIMIZED && g_device) {
					g_resizeWidth = static_cast<UINT>(LOWORD(lParam));
					g_resizeHeight = static_cast<UINT>(HIWORD(lParam));
				}
				return 0;
			case WM_SYSCOMMAND:
				if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
				break;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			}
			return DefWindowProcW(window, message, wParam, lParam);
		}

		std::string ToUtf8(const std::filesystem::path& path) {
			const auto value = path.u8string();
			return std::string(value.begin(), value.end());
		}

		void ApplyStyle() {
			ImGui::StyleColorsDark();
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowRounding = 10.0f;
			style.ChildRounding = 8.0f;
			style.FrameRounding = 6.0f;
			style.PopupRounding = 8.0f;
			style.ScrollbarRounding = 8.0f;
			style.GrabRounding = 6.0f;
			style.WindowPadding = ImVec2(18.0f, 16.0f);
			style.FramePadding = ImVec2(12.0f, 8.0f);
			style.ItemSpacing = ImVec2(10.0f, 9.0f);
			auto& colors = style.Colors;
			colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.065f, 0.075f, 1.0f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.075f, 0.088f, 0.10f, 1.0f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.105f, 0.12f, 0.98f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.15f, 0.17f, 1.0f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.21f, 0.22f, 1.0f);
			colors[ImGuiCol_Button] = ImVec4(0.12f, 0.48f, 0.35f, 1.0f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.16f, 0.60f, 0.43f, 1.0f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.40f, 0.29f, 1.0f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.82f, 0.58f, 1.0f);
			colors[ImGuiCol_Header] = ImVec4(0.12f, 0.27f, 0.22f, 1.0f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.36f, 0.28f, 1.0f);
			colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.13f, 0.14f, 1.0f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.42f, 0.32f, 1.0f);
			colors[ImGuiCol_TabSelected] = ImVec4(0.12f, 0.25f, 0.21f, 1.0f);
		}

		void DrawLibrary(FilesContainer& files, Player& player, ImGuiID dockspace) {
			ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
			if (!ImGui::Begin("Library")) { ImGui::End(); return; }
			ImGui::TextDisabled("YOUR MUSIC");
			ImGui::Separator();
			ImGui::Text("All songs");
			ImGui::SameLine();
			ImGui::TextDisabled("%d tracks", static_cast<int>(files.getFiles()->size()));
			ImGui::Spacing();
			ImGui::Columns(2, "library-columns", false);
			ImGui::TextDisabled("TITLE");
			ImGui::NextColumn();
			ImGui::TextDisabled("LOCATION");
			ImGui::NextColumn();
			ImGui::Separator();
			int index = 0;
			for (MusicFile& file : *files.getFiles()) {
				const std::string title = *file.getName();
				const std::string full = ToUtf8(file.getPath()->parent_path());
				const bool selected = player.getCurFile() == &file;
				if (ImGui::Selectable((title + "##track" + std::to_string(index)).c_str(), selected,
					ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
					player.Play(file);
				ImGui::NextColumn();
				ImGui::TextDisabled("%s", full.c_str());
				ImGui::NextColumn();
				++index;
			}
			ImGui::Columns(1);
			if (files.getFiles()->empty()) {
				ImGui::Spacing();
				ImGui::TextWrapped("No music found. Add WAV, MP3, or FLAC files to your music folder, then restart the app.");
			}
			ImGui::End();
		}

		void DrawNowPlaying(Player& player, ImGuiID dockspace) {
			ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
			if (!ImGui::Begin("Now Playing")) { ImGui::End(); return; }
			ImGui::TextDisabled("NOW PLAYING");
			ImGui::Separator();
			MusicFile* current = player.getCurFile();
			if (current) {
				ImGui::Spacing();
				ImGui::TextWrapped("%s", current->getName()->c_str());
				ImGui::TextDisabled("%s", ToUtf8(current->getPath()->parent_path()).c_str());
			}
			else {
				ImGui::TextDisabled("Choose a song from your library to start listening.");
			}
			ImGui::End();
		}

		void DrawPlayback(Player& player, ImGuiID dockspace) {
			ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
			if (!ImGui::Begin("Playback")) { ImGui::End(); return; }
			if (ImGui::Button("|<")) player.PlayPrevious();
			ImGui::SameLine();
			const bool playing = *player.getIsPlaying();
			if (ImGui::Button(playing ? "Pause" : "Play")) {
				if (playing) player.Pause();
				else if (player.getCurFile()) player.Resume();
				else player.PlayNext();
			}
			ImGui::SameLine();
			if (ImGui::Button("|>")) player.PlayNext();
			ImGui::Spacing();
			float volume = player.getVolume();
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.0f%%"))
				player.setVolume(volume);
			ImGui::End();
		}
	}

	Visuals::Visuals(FilesContainer& files, Player& player) : files(files), player(player) {}

	int Visuals::Run() {
		WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, WindowProc, 0, 0, GetModuleHandleW(nullptr), nullptr, nullptr,
			nullptr, nullptr, L"MusicPlayerWindow", nullptr };
		RegisterClassExW(&wc);
		HWND window = CreateWindowW(wc.lpszClassName, L"Music Player", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
		if (!window || !CreateDeviceD3D(window)) {
			CleanupDeviceD3D();
			if (window) DestroyWindow(window);
			UnregisterClassW(wc.lpszClassName, wc.hInstance);
			return 1;
		}

		ShowWindow(window, SW_SHOWDEFAULT);
		UpdateWindow(window);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
		ApplyStyle();
		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(g_device, g_context);

		bool running = true;
		MSG message{};
		while (running) {
			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&message);
				DispatchMessageW(&message);
				if (message.message == WM_QUIT) running = false;
			}
			if (!running) break;
			if (g_resizeWidth && g_resizeHeight) {
				CleanupRenderTarget();
				g_swapChain->ResizeBuffers(0, g_resizeWidth, g_resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
				g_resizeWidth = g_resizeHeight = 0;
				CreateRenderTarget();
			}

			player.Update();
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			const ImGuiID dockspace = ImGui::GetID("MusicPlayerDockspace");
			ImGui::DockSpaceOverViewport(dockspace, ImGui::GetMainViewport());
			DrawLibrary(files, player, dockspace);
			DrawNowPlaying(player, dockspace);
			DrawPlayback(player, dockspace);
			ImGui::Render();
			const float clearColor[4] = { 0.055f, 0.065f, 0.075f, 1.0f };
			g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
			g_context->ClearRenderTargetView(g_renderTarget, clearColor);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			g_swapChain->Present(1, 0);
		}

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		CleanupDeviceD3D();
		DestroyWindow(window);
		UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 0;
	}
}
