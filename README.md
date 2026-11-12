# MusicPlayer

MusicPlayer is a C++ music player with a Windows console interface and a Linux terminal interface.

## Build on Linux (including WSL Ubuntu)

Install the compiler and CMake:

```sh
sudo apt update
sudo apt install -y build-essential cmake
```

In WSL, the Windows checkout is available under `/mnt/c`. Build from a copy inside Linux's filesystem. This avoids build and executable permission issues on `/mnt/c`, and using a source path without `#` avoids problems in generated Makefiles (the Windows checkout here is under `C#Projects`).

```sh
mkdir -p ~/MusicPlayer-src
cp -a '/mnt/c/C#Projects/MusicPlayer/.' ~/MusicPlayer-src/
cmake -S ~/MusicPlayer-src -B ~/MusicPlayer-linux-build -DCMAKE_BUILD_TYPE=Release
cmake --build ~/MusicPlayer-linux-build --parallel
```

The executable is saved in your Ubuntu home folder (`~`):

```text
~/MusicPlayer-linux-build/MusicPlayer
```

Run it from Ubuntu with:

```sh
~/MusicPlayer-linux-build/MusicPlayer
```

When you change the Windows checkout and want to rebuild, repeat the `cp -a` command to update the Linux source copy, then run the two CMake commands again.

## Copy the Linux executable to Windows

To put a shareable copy next to the project in Windows, run:

```sh
cp ~/MusicPlayer-linux-build/MusicPlayer '/mnt/c/C#Projects/MusicPlayer/MusicPlayer-linux'
```

The file will be at `C:\C#Projects\MusicPlayer\MusicPlayer-linux`. Send that executable to friends using Linux. It is a Linux binary and will not run natively on Windows. On their Linux system, they can run:

```sh
chmod +x MusicPlayer-linux
./MusicPlayer-linux
```

The player needs a working Linux audio output (such as ALSA or PulseAudio/PipeWire). Its Linux terminal controls are W/S to select, Enter to play, Space to pause/resume, N/P for next/previous, +/- for volume, C to change the music directory, `/` to search, R to refresh, and Q to quit. Settings are stored at `$XDG_CONFIG_HOME/eugeniy3339/MusicPlayer/Data.json`, or `$HOME/.config/eugeniy3339/MusicPlayer/Data.json` when `XDG_CONFIG_HOME` is unset.
