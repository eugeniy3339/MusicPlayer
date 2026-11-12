# Linux build

Requirements: CMake 3.16+, a C++20 compiler, and the usual Linux audio development/runtime support used by miniaudio (ALSA or PulseAudio).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/MusicPlayer
```

The Linux terminal interface uses W/S to select, Enter to play, Space to pause/resume, N/P for next/previous, +/- for volume, C to change the music directory, / to search, R to refresh, and Q to quit. Settings are stored at `$XDG_CONFIG_HOME/eugeniy3339/MusicPlayer/Data.json`, or `$HOME/.config/eugeniy3339/MusicPlayer/Data.json` when XDG_CONFIG_HOME is unset.
