#pragma once

#include "musicplayerfunctions.h"

namespace MusicPlayer {

	std::wstring Utf8ToWide(const std::string& value) {
		if (value.empty())
			return {};

		const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(),
			static_cast<int>(value.size()), nullptr, 0);
		if (size <= 0)
			return std::wstring(value.begin(), value.end());

		std::wstring result(size, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
			result.data(), size);
		return result;
	}

	std::wstring Lowercase(std::wstring value) {
		if (value.empty())
			return value;
		const int size = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
			value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr, 0);
		if (size <= 0)
			return value;
		std::wstring result(size, L'\0');
		LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, value.data(),
			static_cast<int>(value.size()), result.data(), size, nullptr, nullptr, 0);
		return result;
	}

}