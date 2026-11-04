#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>

#include "Scripts/Data/FilesContainer.h"
#include "Scripts/Logic/Player.h"
#include "Scripts/Presentation/MusicPlayerView.h"
#include "external/imgui/imgui.h"
#include "external/imgui/backends/imgui_impl_dx11.h"
#include "external/imgui/backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

namespace {
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* renderTarget = nullptr;

	void CreateRenderTarget() {
		ID3D11Texture2D* backBuffer = nullptr;
		if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
			device->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
			backBuffer->Release();
		}
	}

	void DestroyRenderTarget() {
		if (renderTarget != nullptr) {
			renderTarget->Release();
			renderTarget = nullptr;
		}
	}

	bool CreateDeviceD3D(HWND window) {
		DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
		swapChainDescription.BufferCount = 2;
		swapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescription.OutputWindow = window;
		swapChainDescription.SampleDesc.Count = 1;
		swapChainDescription.Windowed = TRUE;
		swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		const D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_0
		};
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		const HRESULT result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			featureLevels,
			static_cast<UINT>(sizeof(featureLevels) / sizeof(featureLevels[0])),
			D3D11_SDK_VERSION,
			&swapChainDescription,
			&swapChain,
			&device,
			&featureLevel,
			&deviceContext);
		if (FAILED(result))
			return false;

		CreateRenderTarget();
		return renderTarget != nullptr;
	}

	void DestroyDeviceD3D() {
		DestroyRenderTarget();
		if (swapChain != nullptr) {
			swapChain->Release();
			swapChain = nullptr;
		}
		if (deviceContext != nullptr) {
			deviceContext->Release();
			deviceContext = nullptr;
		}
		if (device != nullptr) {
			device->Release();
			device = nullptr;
		}
	}

	LRESULT WINAPI WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
		if (ImGui::GetCurrentContext() != nullptr &&
			ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam))
			return 1;

		switch (message) {
		case WM_SIZE:
			if (device != nullptr && wParam != SIZE_MINIMIZED) {
				DestroyRenderTarget();
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

int main() {
	ImGui_ImplWin32_EnableDpiAwareness();

	const wchar_t* windowClassName = L"MusicPlayerWindowClass";
	WNDCLASSEXW windowClass = {};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcedure;
	windowClass.hInstance = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = windowClassName;
	RegisterClassExW(&windowClass);

	HWND window = CreateWindowW(
		windowClassName,
		L"Music Player",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		760,
		720,
		nullptr,
		nullptr,
		windowClass.hInstance,
		nullptr);
	if (window == nullptr || !CreateDeviceD3D(window)) {
		std::cerr << "Failed to create the music player window or Direct3D device.\n";
		if (window != nullptr)
			DestroyWindow(window);
		DestroyDeviceD3D();
		UnregisterClassW(windowClassName, windowClass.hInstance);
		return 1;
	}

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, deviceContext);

	MusicPlayer::FilesContainer filesContainer;
	MusicPlayer::Player player;
	MusicPlayer::MusicPlayerView view;
	bool running = true;
	while (running) {
		MSG message;
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
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
		view.Render(filesContainer, player);
		ImGui::Render();

		const float clearColor[] = { 0.055f, 0.063f, 0.078f, 1.0f };
		deviceContext->OMSetRenderTargets(1, &renderTarget, nullptr);
		deviceContext->ClearRenderTargetView(renderTarget, clearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swapChain->Present(1, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	DestroyDeviceD3D();
	DestroyWindow(window);
	UnregisterClassW(windowClassName, windowClass.hInstance);
	return 0;
}
