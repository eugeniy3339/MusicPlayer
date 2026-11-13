# Project rules

- Keep application code in `Scripts/`, grouped by responsibility; keep the process entry point in `main.cpp`.
- Use C++20 and the existing Visual Studio project. Add every new `.cpp` and `.h` file to both `MusicPlayer.vcxproj` and `MusicPlayer.vcxproj.filters`.
- Put application code in the `MusicPlayer` namespace. Match the existing style: PascalCase for actions and types, lower camel case for fields and local variables.
- Keep Dear ImGui calls in `Scripts/Presentation/`. Keep the music library and its player controls in one app window, with playback controls along the bottom.
- Keep platform/window/rendering setup behind the presentation layer; do not put UI code in playback or file-data classes.
- Keep vendored third-party source under `external/` and record its upstream branch/revision in an `UPSTREAM.md` file.
- Record significant architecture choices in `docs/adr/` using sequentially numbered ADRs.
- Preserve the existing playback and library APIs when building UI; route user actions through those APIs.
