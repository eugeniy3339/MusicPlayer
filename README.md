# Music Player

Music Player is a native Windows desktop application for browsing and playing a local music collection. It uses a single window for the music library and playback controls, with optional views for embedded cover art and lyrics.

## Features

- Browse music files in a configurable folder and refresh the library after making changes.
- Search tracks by filename, with live filtering and a match count.
- Play, pause, stop, and move to the previous or next track; adjust playback volume.
- Read track metadata, including title, artist, album, genre, track and disc numbers, composer, year, comments, and descriptions.
- Switch between a compact title list and a list with embedded cover art.
- Remember the selected library display mode between runs.
- Show embedded lyrics for the currently playing track.
- Save the music folder and volume between runs.
- Display Unicode text using available system fonts.

The library currently scans `.wav`, `.mp3`, and `.flac` files.

## Build and run

1. Open `MusicPlayer.sln` in Visual Studio 2022 with the **Desktop development with C++** workload installed.
2. Select the **x64** platform and either the **Debug** or **Release** configuration.
3. Build and run the `MusicPlayer` project.

The project uses C++20 and the Visual Studio v143 toolset. It includes the Win32 and Direct3D 11 Dear ImGui backends and links against the Windows Direct3D libraries. No separate package installation is needed for the vendored dependencies.

On first launch, the app uses `C:\Music` as its default music folder. Change it in **Settings** if your files are elsewhere. The selected folder and playback volume are saved for future runs.

## Project layout

- `main.cpp` creates the application services and starts the presentation layer.
- `Scripts/Data/` handles the music library, saved settings, and audio metadata reading.
- `Scripts/Logic/` contains playback logic.
- `Scripts/Commands/` contains command and event handling.
- `Scripts/Presentation/` contains the Dear ImGui interface and Windows rendering setup.
- `external/` contains vendored dependencies, including Dear ImGui and miniaudio.
- `docs/adr/` records significant architecture decisions.

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui), using the docking branch and the Win32/Direct3D 11 backends.
- [miniaudio](https://github.com/mackron/miniaudio) for audio playback.
- [eventpp](https://github.com/wqking/eventpp) for callback lists.
- [nlohmann/json](https://github.com/nlohmann/json) for saved settings.

See the dependency license files and source headers in `external/` for licensing and upstream details.
