#include "Visuals.h"

#include "../../external/imgui/backends/imgui_impl_dx11.h"
#include "../../external/imgui/backends/imgui_impl_win32.h"
#include "../../external/imgui/imgui.h"

#include <d3d11.h>
#include <windows.h>
#include <algorithm>
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
			style.WindowRounding = 0.0f;
			style.ChildRounding = 4.0f;
			style.FrameRounding = 4.0f;
			style.PopupRounding = 4.0f;
			style.ScrollbarRounding = 4.0f;
			style.GrabRounding = 4.0f;
			style.WindowPadding = ImVec2(28.0f, 24.0f);
			style.FramePadding = ImVec2(12.0f, 9.0f);
			style.ItemSpacing = ImVec2(10.0f, 8.0f);
			style.WindowBorderSize = 0.0f;
			auto& colors = style.Colors;
			colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.035f, 0.035f, 1.0f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.055f, 0.055f, 1.0f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.075f, 0.075f, 1.0f);
			colors[ImGuiCol_Text] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.53f, 0.53f, 0.53f, 1.0f);
			colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
			colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.0f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
			colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
			colors[ImGuiCol_Header] = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.76f, 0.76f, 0.76f, 1.0f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
			colors[ImGuiCol_TableRowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.085f, 0.085f, 0.085f, 1.0f);
		}

		void DrawApp(FilesContainer& files, Player& player) {
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
			constexpr ImGuiWindowFlags appFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
			if (!ImGui::Begin("Music Player", nullptr, appFlags)) {
				ImGui::End();
				return;
			}

			ImGui::TextDisabled("MUSIC PLAYER");
			ImGui::SameLine(0.0f, 20.0f);
			ImGui::TextDisabled("/  YOUR LIBRARY");
			ImGui::Spacing();
			ImGui::SetWindowFontScale(1.35f);
			ImGui::TextUnformatted("All songs");
			ImGui::SetWindowFontScale(1.0f);
			ImGui::SameLine();
			ImGui::TextDisabled("%d tracks", static_cast<int>(files.getFiles()->size()));
			ImGui::Spacing();
			ImGui::Separator();

			constexpr float playerBarHeight = 112.0f;
			if (ImGui::BeginChild("TrackList", ImVec2(0.0f, -playerBarHeight), false)) {
				if (ImGui::BeginTable("Tracks", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
					ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
					ImVec2(0.0f, 0.0f))) {
					ImGui::TableSetupColumn("TITLE", ImGuiTableColumnFlags_WidthStretch, 0.68f);
					ImGui::TableSetupColumn("LOCATION", ImGuiTableColumnFlags_WidthStretch, 0.32f);
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableHeadersRow();
					constexpr float trackRowHeight = 42.0f;
					int index = 0;
					for (MusicFile& file : *files.getFiles()) {
						const std::string title = *file.getName();
						const std::string location = ToUtf8(file.getPath()->parent_path());
						const bool selected = player.getCurFile() == &file;
						ImGui::TableNextRow(ImGuiTableRowFlags_None, trackRowHeight);
						ImGui::TableSetColumnIndex(0);
						if (ImGui::Selectable((title + "##track" + std::to_string(index)).c_str(), selected,
							ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, trackRowHeight)))
							player.Play(file);
						ImGui::TableSetColumnIndex(1);
						ImGui::TextDisabled("%s", location.c_str());
						++index;
					}
					ImGui::EndTable();
				}
				if (files.getFiles()->empty()) {
					ImGui::Spacing();
					ImGui::TextDisabled("No music found. Add WAV, MP3, or FLAC files to your music folder.");
				}
			}
			ImGui::EndChild();

			if (ImGui::BeginChild("PlayerBar", ImVec2(0.0f, 0.0f), true,
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
				if (ImGui::BeginTable("PlayerBarLayout", 3, ImGuiTableFlags_SizingStretchProp)) {
					ImGui::TableSetupColumn("CurrentTrack", ImGuiTableColumnFlags_WidthStretch, 0.34f);
					ImGui::TableSetupColumn("Transport", ImGuiTableColumnFlags_WidthStretch, 0.36f);
					ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthStretch, 0.30f);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextDisabled("NOW PLAYING");
					if (MusicFile* current = player.getCurFile()) {
						ImGui::Text("%s", current->getName()->c_str());
						ImGui::TextDisabled("%s", ToUtf8(current->getPath()->parent_path()).c_str());
					}
					else {
						ImGui::TextDisabled("Nothing playing");
					}

					ImGui::TableSetColumnIndex(1);
					const float buttonsWidth = 48.0f * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
					const float centerX = ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - buttonsWidth) * 0.5f;
					ImGui::SetCursorPosX((std::max)(centerX, ImGui::GetCursorPosX()));
					if (ImGui::Button("<<", ImVec2(48.0f, 38.0f))) player.PlayPrevious();
					ImGui::SameLine();
					const bool playing = *player.getIsPlaying();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.94f, 0.94f, 0.94f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.78f, 0.78f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.64f, 0.64f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
					if (ImGui::Button(playing ? "||" : ">", ImVec2(48.0f, 38.0f))) {
						if (playing) player.Pause();
						else if (player.getCurFile()) player.Resume();
						else player.PlayNext();
					}
					ImGui::PopStyleColor(4);
					ImGui::SameLine();
					if (ImGui::Button(">>", ImVec2(48.0f, 38.0f))) player.PlayNext();

					ImGui::TableSetColumnIndex(2);
					ImGui::TextDisabled("VOLUME");
					float volumePercent = player.getVolume() * 100.0f;
					ImGui::SetNextItemWidth(-1.0f);
					if (ImGui::SliderFloat("##Volume", &volumePercent, 0.0f, 100.0f, "%.0f%%"))
						player.setVolume(volumePercent / 100.0f);
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();
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
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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
			DrawApp(files, player);
			ImGui::Render();
			const float clearColor[4] = { 0.035f, 0.035f, 0.035f, 1.0f };
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
