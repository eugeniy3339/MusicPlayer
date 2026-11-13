# ADR 0001: Use Dear ImGui docking for the music player UI

- Status: Superseded by [ADR 0002](0002-single-window-player-layout.md)
- Date: 2026-11-13

## Context

The application is a C++20 Windows music player with Dear ImGui vendored in `external/imgui`. The existing project compiles the Win32 and DirectX 11 backends. The product needs a desktop GUI whose primary screens can be rearranged and docked instead of opening as floating popups.

## Decision

Use the official Dear ImGui `docking` branch, enable `ImGuiConfigFlags_DockingEnable`, and create a persistent dockspace in the main viewport. Place the Library, Now Playing, and Playback screens in that dockspace on first launch. Keep those screens as regular ImGui windows so users can rearrange them and ImGui can persist their layout in `imgui.ini`.

## Consequences

- The app uses the docking branch API and should track the docking branch when updating Dear ImGui.
- The current application build remains Windows/Win32/DirectX 11; other platform backends require separate platform build work.
- ImGui window layout is user-adjustable and persisted by Dear ImGui.
