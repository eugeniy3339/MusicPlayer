#pragma once

#include <string>
#include <windows.h>

namespace MusicPlayer {
	std::wstring Utf8ToWide(const std::string& value);
	std::wstring Lowercase(std::wstring value);
}