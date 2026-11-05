#include "Visuals.h"

#include "../../external/imgui/imgui.h"
#include "../../external/imgui/backends/imgui_impl_dx11.h"
#include "../../external/imgui/backends/imgui_impl_win32.h"
#include <windows.h>
#include <d3d11.h>
#include <iterator>
#include <tchar.h>

// The backend header intentionally omits this declaration to avoid including
// Windows headers in every translation unit that uses it.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

namespace {
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* renderTargetView = nullptr;

	void CreateRenderTarget() {
		ID3D11Texture2D* backBuffer = nullptr;
		if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
			device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
			backBuffer->Release();
		}
	}

	void CleanupRenderTarget() {
		if (renderTargetView) {
			renderTargetView->Release();
			renderTargetView = nullptr;
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
		D3D_FEATURE_LEVEL selectedLevel{};
		const HRESULT result = D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels,
			static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION,
			&desc, &swapChain, &device, &selectedLevel, &deviceContext);
		if (FAILED(result))
			return false;
		CreateRenderTarget();
		return renderTargetView != nullptr;
	}

	void CleanupDeviceD3D() {
		CleanupRenderTarget();
		if (swapChain) { swapChain->Release(); swapChain = nullptr; }
		if (deviceContext) { deviceContext->Release(); deviceContext = nullptr; }
		if (device) { device->Release(); device = nullptr; }
	}

	LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam))
			return 1;
		switch (message) {
		case WM_SIZE:
			if (device && wParam != SIZE_MINIMIZED) {
				CleanupRenderTarget();
				swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

namespace MusicPlayer {
	Visuals::Visuals(Player& player, FilesContainer& files) : view(player, files) {
		const HINSTANCE instance = GetModuleHandleW(nullptr);
		WNDCLASSEXW windowClass{ sizeof(windowClass), CS_CLASSDC, WindowProc, 0, 0,
			instance, nullptr, nullptr, nullptr, nullptr, L"MusicPlayerWindow", nullptr };
		RegisterClassExW(&windowClass);
		HWND window = CreateWindowW(windowClass.lpszClassName, L"Music Player",
			WS_OVERLAPPEDWINDOW, 100, 100, 1040, 760, nullptr, nullptr, instance, nullptr);
		if (!window)
			return;
		if (!CreateDeviceD3D(window)) {
			CleanupDeviceD3D();
			DestroyWindow(window);
			UnregisterClassW(windowClass.lpszClassName, instance);
			return;
		}

		ShowWindow(window, SW_SHOWDEFAULT);
		UpdateWindow(window);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;
		colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.065f, 0.085f, 1.0f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.075f, 0.090f, 0.115f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.13f, 0.30f, 0.34f, 0.75f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.16f, 0.43f, 0.46f, 0.85f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.50f, 0.52f, 1.0f);
		colors[ImGuiCol_Button] = ImVec4(0.12f, 0.28f, 0.32f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.40f, 0.43f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.49f, 0.51f, 1.0f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.0f);
		style.WindowRounding = 10.0f;
		style.FrameRounding = 6.0f;
		style.ChildRounding = 8.0f;
		style.ItemSpacing = ImVec2(10.0f, 8.0f);
		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(device, deviceContext);

		bool running = true;
		while (running) {
			MSG message;
			while (PeekMessageW(&message, nullptr, 0U, 0U, PM_REMOVE)) {
				TranslateMessage(&message);
				DispatchMessageW(&message);
				if (message.message == WM_QUIT)
					running = false;
			}
			if (!running)
				break;

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			view.Draw();
			ImGui::Render();
			const float clearColor[4] = { 0.055f, 0.065f, 0.085f, 1.0f };
			deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
			deviceContext->ClearRenderTargetView(renderTargetView, clearColor);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			swapChain->Present(1, 0);
		}

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		CleanupDeviceD3D();
		DestroyWindow(window);
		UnregisterClassW(windowClass.lpszClassName, instance);
	}
}
