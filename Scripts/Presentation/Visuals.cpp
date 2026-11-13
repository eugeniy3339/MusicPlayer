#include "Visuals.h"

#include "../../external/imgui/backends/imgui_impl_dx11.h"
#include "../../external/imgui/backends/imgui_impl_win32.h"
#include "../../external/imgui/imgui.h"
#include "../musicplayerfunctions.h"

#include <d3d11.h>
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace MusicPlayer {
	namespace {
		ID3D11Device* g_device = nullptr;
		ID3D11DeviceContext* g_context = nullptr;
		IDXGISwapChain* g_swapChain = nullptr;
		ID3D11RenderTargetView* g_renderTarget = nullptr;
		UINT g_resizeWidth = 0;
		UINT g_resizeHeight = 0;
		struct CoverArtTextureEntry {
			ID3D11ShaderResourceView* texture = nullptr;
			int lastUsedFrame = 0;
		};
		std::unordered_map<std::string, CoverArtTextureEntry> g_coverArtTextures;

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

		ID3D11ShaderResourceView* CreateCoverArtTexture(const std::vector<std::uint8_t>& imageBytes) {
			if (imageBytes.empty() || imageBytes.size() > MAXDWORD)
				return nullptr;

			using Microsoft::WRL::ComPtr;
			ComPtr<IWICImagingFactory> factory;
			if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(factory.GetAddressOf()))))
				return nullptr;

			ComPtr<IWICStream> stream;
			if (FAILED(factory->CreateStream(stream.GetAddressOf())) ||
				FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(imageBytes.data()), static_cast<DWORD>(imageBytes.size()))))
				return nullptr;

			ComPtr<IWICBitmapDecoder> decoder;
			if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad,
				decoder.GetAddressOf())))
				return nullptr;

			ComPtr<IWICBitmapFrameDecode> frame;
			if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
				return nullptr;

			UINT width = 0;
			UINT height = 0;
			if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0 || width > 4096 || height > 4096)
				return nullptr;

			ComPtr<IWICFormatConverter> converter;
			constexpr UINT maxCoverTextureSize = 512;
			const float scale = (std::min)(1.0f, static_cast<float>(maxCoverTextureSize) /
				static_cast<float>((std::max)(width, height)));
			const UINT outputWidth = (std::max)(1u, static_cast<UINT>(width * scale));
			const UINT outputHeight = (std::max)(1u, static_cast<UINT>(height * scale));
			ComPtr<IWICBitmapScaler> scaler;
			IWICBitmapSource* imageSource = frame.Get();
			if ((outputWidth != width || outputHeight != height) &&
				SUCCEEDED(factory->CreateBitmapScaler(scaler.GetAddressOf())) &&
				SUCCEEDED(scaler->Initialize(frame.Get(), outputWidth, outputHeight, WICBitmapInterpolationModeFant)))
				imageSource = scaler.Get();

			if (imageSource == frame.Get()) {
				// Keep large source dimensions from allocating oversized GPU textures.
				if (outputWidth != width || outputHeight != height)
					return nullptr;
			}
			if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
				FAILED(converter->Initialize(imageSource, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
					nullptr, 0.0, WICBitmapPaletteTypeCustom)))
				return nullptr;
			const UINT rowPitch = outputWidth * 4;
			std::vector<BYTE> pixels(static_cast<std::size_t>(rowPitch) * outputHeight);
			if (FAILED(converter->CopyPixels(nullptr, rowPitch, static_cast<UINT>(pixels.size()), pixels.data())))
				return nullptr;

			D3D11_TEXTURE2D_DESC textureDesc{};
			textureDesc.Width = outputWidth;
			textureDesc.Height = outputHeight;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			D3D11_SUBRESOURCE_DATA initialData{};
			initialData.pSysMem = pixels.data();
			initialData.SysMemPitch = rowPitch;

			ComPtr<ID3D11Texture2D> texture;
			if (FAILED(g_device->CreateTexture2D(&textureDesc, &initialData, texture.GetAddressOf())))
				return nullptr;
			ID3D11ShaderResourceView* textureView = nullptr;
			if (FAILED(g_device->CreateShaderResourceView(texture.Get(), nullptr, &textureView)))
				return nullptr;
			return textureView;
		}

		ID3D11ShaderResourceView* GetCoverArtTexture(MusicFile& file) {
			const auto& imageBytes = file.getMetadata().getCoverArt();
			if (imageBytes.empty())
				return nullptr;
			const std::string key = ToUtf8(*file.getPath());
			const auto cached = g_coverArtTextures.find(key);
			if (cached != g_coverArtTextures.end()) {
				cached->second.lastUsedFrame = ImGui::GetFrameCount();
				return cached->second.texture;
			}
			constexpr std::size_t maxCachedCoverArts = 32;
			const int currentFrame = ImGui::GetFrameCount();
			if (g_coverArtTextures.size() >= maxCachedCoverArts) {
				auto oldest = g_coverArtTextures.end();
				for (auto entry = g_coverArtTextures.begin(); entry != g_coverArtTextures.end(); ++entry) {
					if (entry->second.lastUsedFrame == currentFrame)
						continue;
					if (oldest == g_coverArtTextures.end() || entry->second.lastUsedFrame < oldest->second.lastUsedFrame)
						oldest = entry;
				}
				if (oldest != g_coverArtTextures.end()) {
					if (oldest->second.texture)
						oldest->second.texture->Release();
					g_coverArtTextures.erase(oldest);
				}
			}
			ID3D11ShaderResourceView* texture = CreateCoverArtTexture(imageBytes);
			g_coverArtTextures.emplace(key, CoverArtTextureEntry{ texture, currentFrame });
			return texture;
		}

		void ClearCoverArtTextures() {
			for (auto& [path, entry] : g_coverArtTextures) {
				if (entry.texture)
					entry.texture->Release();
			}
			g_coverArtTextures.clear();
		}

		ImFont* LoadFirstAvailableFont(ImGuiIO& io, std::initializer_list<const char*> paths,
			const ImFontConfig* config = nullptr) {
			for (const char* path : paths) {
				std::error_code error;
				if (!std::filesystem::is_regular_file(path, error))
					continue;

				ImFontConfig fontConfig = config ? *config : ImFontConfig();
				fontConfig.Flags |= ImFontFlags_NoLoadError;
				if (ImFont* font = io.Fonts->AddFontFromFileTTF(path, 0.0f, &fontConfig))
					return font;
			}
			return nullptr;
		}

		void LoadUnicodeFonts(ImGuiIO& io) {
			ImFont* baseFont = LoadFirstAvailableFont(io, {
				"C:\\Windows\\Fonts\\segoeui.ttf",
				"C:\\Windows\\Fonts\\arial.ttf",
				"C:\\Windows\\Fonts\\tahoma.ttf"
			});
			if (!baseFont)
				baseFont = io.Fonts->AddFontDefaultVector();
			io.FontDefault = baseFont;

			ImFontConfig mergeConfig;
			mergeConfig.MergeMode = true;
			mergeConfig.Flags |= ImFontFlags_NoLoadError;
			LoadFirstAvailableFont(io, {
				"C:\\Windows\\Fonts\\arial.ttf",
				"C:\\Windows\\Fonts\\tahoma.ttf"
			}, &mergeConfig);
			LoadFirstAvailableFont(io, {
				"C:\\Windows\\Fonts\\msgothic.ttc",
				"C:\\Windows\\Fonts\\YuGothM.ttc",
				"C:\\Windows\\Fonts\\meiryo.ttc"
			}, &mergeConfig);
			LoadFirstAvailableFont(io, {
				"C:\\Windows\\Fonts\\msyh.ttc",
				"C:\\Windows\\Fonts\\simsun.ttc"
			}, &mergeConfig);
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

		void DrawApp(FilesContainer& files, Player& player, Saver& saver, bool& menuOpen, bool& settingsOpen,
			bool& lyricsOpen, bool& showCoverArt, bool& invalidMusicPath,
			std::array<char, 512>& searchInput, std::unordered_set<std::filesystem::path>& searchResults,
			std::string& lastSearchQuery, bool& searchResultsDirty,
			std::array<char, 1024>& musicPathInput) {
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

			if (ImGui::Button(menuOpen ? "CLOSE MENU" : "MENU", ImVec2(112.0f, 36.0f)))
				menuOpen = !menuOpen;
			ImGui::SameLine(0.0f, 8.0f);
			if (ImGui::Button(lyricsOpen ? "HIDE LYRICS" : "LYRICS", ImVec2(124.0f, 36.0f)))
				lyricsOpen = !lyricsOpen;
			ImGui::SameLine(0.0f, 18.0f);
			ImGui::TextDisabled("MUSIC PLAYER");
			ImGui::Spacing();
			ImGui::Separator();

			constexpr float playerBarHeight = 112.0f;
			const float contentHeight = (std::max)(0.0f, ImGui::GetContentRegionAvail().y - playerBarHeight);
			if (ImGui::BeginChild("ContentArea", ImVec2(0.0f, contentHeight), false)) {
				if (menuOpen) {
					if (ImGui::BeginChild("SideMenu", ImVec2(220.0f, 0.0f), true)) {
						ImGui::TextDisabled("MENU");
						ImGui::Separator();
						if (ImGui::Button("Library", ImVec2(-1.0f, 42.0f)))
							settingsOpen = false;
						if (ImGui::Button("Settings", ImVec2(-1.0f, 42.0f)))
							settingsOpen = true;
					}
					ImGui::EndChild();
					ImGui::SameLine();
				}

				constexpr float lyricsPanelWidth = 280.0f;
				const float mainPanelWidth = lyricsOpen
					? (std::max)(0.0f, ImGui::GetContentRegionAvail().x - lyricsPanelWidth - ImGui::GetStyle().ItemSpacing.x)
					: 0.0f;
				if (ImGui::BeginChild("MainPanel", ImVec2(mainPanelWidth, 0.0f), false)) {
					if (settingsOpen) {
						ImGui::SetWindowFontScale(1.35f);
						ImGui::TextUnformatted("Settings");
						ImGui::SetWindowFontScale(1.0f);
						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();
						ImGui::Text("Music folder");
						ImGui::SetNextItemWidth(-1.0f);
						ImGui::InputText("##MusicFolderPath", musicPathInput.data(), musicPathInput.size());
						ImGui::TextDisabled("Enter the full path to the folder containing your music.");
						ImGui::Spacing();
						if (ImGui::Button("Save folder", ImVec2(140.0f, 40.0f))) {
							const std::string path(musicPathInput.data());
							const std::filesystem::path folder(std::u8string(path.begin(), path.end()));
							std::error_code error;
							if (!path.empty() && std::filesystem::is_directory(folder, error)) {
								player.Stop();
								ClearCoverArtTextures();
								files.SetPathToMusicDirectory(std::string(path));
								invalidMusicPath = false;
								searchResultsDirty = true;
							}
							else {
								invalidMusicPath = true;
							}
						}
						if (invalidMusicPath) {
							ImGui::SameLine();
							ImGui::TextColored(ImVec4(0.9f, 0.55f, 0.55f, 1.0f), "Folder not found. Check the path and try again.");
						}
					}
					else {
						ImGui::SetNextItemWidth(-1.0f);
						if (ImGui::InputTextWithHint("##TrackSearch", "Search your music", searchInput.data(), searchInput.size()))
							searchResultsDirty = true;
						const std::string searchQuery(searchInput.data());
						auto updateSearchResults = [&]() {
							if (!searchResultsDirty && searchQuery == lastSearchQuery)
								return;
							lastSearchQuery = searchQuery;
							searchResults.clear();
							if (!searchQuery.empty()) {
								std::vector<MusicFile> matchingFiles = files.GetMatchingFiles(Utf8ToWide(searchQuery));
								for (MusicFile& matchingFile : matchingFiles)
									searchResults.insert(*matchingFile.getPath());
							}
							searchResultsDirty = false;
						};
						updateSearchResults();
						ImGui::Spacing();
						ImGui::SetWindowFontScale(1.35f);
						ImGui::TextUnformatted("All songs");
						ImGui::SetWindowFontScale(1.0f);
						ImGui::SameLine();
						if (searchQuery.empty())
							ImGui::TextDisabled("%d tracks", static_cast<int>(files.getFiles()->size()));
						else
							ImGui::TextDisabled("%d matches", static_cast<int>(searchResults.size()));
						ImGui::SameLine(0.0f, 14.0f);
						if (ImGui::Button("REFRESH")) {
							ClearCoverArtTextures();
							files.RefreshList();
							searchResultsDirty = true;
						}
						updateSearchResults();
						ImGui::SameLine(0.0f, 20.0f);
						const bool titleOnlyActive = !showCoverArt;
						if (titleOnlyActive) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.94f, 0.94f, 0.94f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
						}
						const bool titleOnlyClicked = ImGui::Button("TITLE ONLY");
						if (titleOnlyActive)
							ImGui::PopStyleColor(2);
						ImGui::SameLine();
						const bool coverArtActive = showCoverArt;
						if (coverArtActive) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.94f, 0.94f, 0.94f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
						}
						const bool coverArtClicked = ImGui::Button("TITLE + COVER");
						if (coverArtActive)
							ImGui::PopStyleColor(2);
						if (titleOnlyClicked) {
							showCoverArt = false;
							int showingType = 0;
							saver.SaveFilesListShowingType(showingType);
						}
						else if (coverArtClicked) {
							showCoverArt = true;
							int showingType = 1;
							saver.SaveFilesListShowingType(showingType);
						}
						ImGui::Spacing();
						const int columnCount = showCoverArt ? 2 : 1;
						const char* tableId = showCoverArt ? "TracksWithCoverArt" : "TracksTitleOnly";
						if (ImGui::BeginTable(tableId, columnCount, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
							ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY)) {
							if (showCoverArt) {
								ImGui::TableSetupColumn("COVER", ImGuiTableColumnFlags_WidthFixed, 68.0f);
								ImGui::TableSetupColumn("TITLE", ImGuiTableColumnFlags_WidthStretch, 1.0f);
							}
							else {
								ImGui::TableSetupColumn("TITLE", ImGuiTableColumnFlags_WidthStretch, 1.0f);
							}
							ImGui::TableSetupScrollFreeze(0, 1);
							ImGui::TableHeadersRow();
							int index = 0;
							for (MusicFile& file : *files.getFiles()) {
								if (!searchQuery.empty() && !searchResults.contains(*file.getPath()))
									continue;
								const std::string title = *file.getName();
								const bool selected = player.getCurFile() == &file;
								const float trackRowHeight = showCoverArt ? 64.0f : 42.0f;
								ImGui::TableNextRow(ImGuiTableRowFlags_None, trackRowHeight);
								ImGui::TableSetColumnIndex(0);
								if (showCoverArt) {
									const ImVec2 cellPosition = ImGui::GetCursorScreenPos();
									if (ImGui::Selectable(("##track" + std::to_string(index)).c_str(), selected,
										ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, trackRowHeight)))
										player.Play(file);
									const float imageSize = 46.0f;
									const ImVec2 imageMin(cellPosition.x + 10.0f, cellPosition.y + (trackRowHeight - imageSize) * 0.5f);
									const ImVec2 imageMax(imageMin.x + imageSize, imageMin.y + imageSize);
									if (ImGui::IsItemVisible()) {
										if (ID3D11ShaderResourceView* cover = GetCoverArtTexture(file))
											ImGui::GetWindowDrawList()->AddImage(ImTextureRef(cover), imageMin, imageMax);
										else {
											ImDrawList* drawList = ImGui::GetWindowDrawList();
											drawList->AddRectFilled(imageMin, imageMax, ImGui::GetColorU32(ImGuiCol_FrameBg), 4.0f);
											const ImVec2 center(imageMin.x + imageSize * 0.5f, imageMin.y + imageSize * 0.5f);
											drawList->AddCircle(center, 12.0f, ImGui::GetColorU32(ImGuiCol_TextDisabled), 0, 2.0f);
											drawList->AddCircleFilled(center, 2.5f, ImGui::GetColorU32(ImGuiCol_TextDisabled));
										}
									}
									ImGui::TableSetColumnIndex(1);
									ImGui::Text("%s", title.c_str());
								}
								else {
									if (ImGui::Selectable((title + "##track" + std::to_string(index)).c_str(), selected,
										ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, trackRowHeight)))
										player.Play(file);
								}
								++index;
							}
							ImGui::EndTable();
						}
						if (files.getFiles()->empty()) {
							ImGui::Spacing();
							ImGui::TextDisabled("No music found. Add WAV, MP3, or FLAC files to your music folder.");
						}
						else if (!searchQuery.empty() && searchResults.empty()) {
							ImGui::Spacing();
							ImGui::TextDisabled("No tracks match your search.");
						}
					}
				}
				ImGui::EndChild();
				if (lyricsOpen) {
					ImGui::SameLine();
					if (ImGui::BeginChild("LyricsPanel", ImVec2(lyricsPanelWidth, 0.0f), true)) {
						ImGui::TextDisabled("LYRICS");
						ImGui::Separator();
						if (MusicFile* current = player.getCurFile()) {
							ImGui::TextWrapped("%s", current->getName()->c_str());
							const std::string& lyrics = current->getMetadata().getLyrics();
							ImGui::Spacing();
							ImGui::Separator();
							if (lyrics.empty())
								ImGui::TextDisabled("No lyrics are available for this track.");
							else
								ImGui::TextWrapped("%s", lyrics.c_str());
						}
						else {
							ImGui::TextDisabled("Start a track to see its lyrics.");
						}
					}
					ImGui::EndChild();
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

	Visuals::Visuals(FilesContainer& files, Player& player, Saver& saver) : files(files), player(player), saver(saver) {
		showCoverArt = saver.GetFilesListShowingType() == 1;
		const std::string& musicPath = *files.GetPathToMusicDirectory();
		const size_t length = (std::min)(musicPath.size(), musicPathInput.size() - 1);
		std::copy_n(musicPath.data(), length, musicPathInput.data());
		musicPathInput[length] = '\0';
	}

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
		const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		const bool uninitializeCom = SUCCEEDED(comResult);

		ShowWindow(window, SW_SHOWDEFAULT);
		UpdateWindow(window);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		LoadUnicodeFonts(io);
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
			DrawApp(files, player, saver, menuOpen, settingsOpen, lyricsOpen, showCoverArt,
				invalidMusicPath, searchInput, searchResults, lastSearchQuery, searchResultsDirty,
				musicPathInput);
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
		ClearCoverArtTextures();
		CleanupDeviceD3D();
		if (uninitializeCom)
			CoUninitialize();
		DestroyWindow(window);
		UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 0;
	}
}
