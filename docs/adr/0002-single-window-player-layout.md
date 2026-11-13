# ADR 0002: Keep the library and playback controls in one window

- Status: Accepted
- Date: 2026-11-13
- Supersedes: [ADR 0001](0001-dear-imgui-docking.md)

## Context

The first GUI iteration placed Library, Now Playing, and Playback in separate dockable ImGui windows. This exposed tabbed panes and made the playback controls feel detached from the library. The desired experience keeps the music list central and playback controls continuously visible at the bottom.

## Decision

Render the app as one fixed main window. Show the library in the main content area and keep current-track information, transport controls, and volume in a bottom player bar. Use a monochrome palette. Do not enable or display a docking layout for these primary controls.

## Consequences

- The layout stays consistent and playback remains visible while browsing the library.
- ImGui docking remains available in the vendored dependency for future use but is not enabled by this screen.
- The current window and rendering implementation remains Win32/DirectX 11.
