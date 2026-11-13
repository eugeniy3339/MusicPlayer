#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace MusicPlayer {
	struct AudioMetadataData {
		std::string title;
		std::string artist;
		std::string album;
		std::string albumArtist;
		std::string genre;
		std::string year;
		int trackNumber = 0;
		int discNumber = 0;
		std::string composer;
		std::string comment;
		std::string description;
		std::string lyrics;
		std::string coverArtMimeType;
		std::vector<std::uint8_t> coverArt;
	};

	class AudioMetadata {
	public:
		AudioMetadata() = default;
		explicit AudioMetadata(AudioMetadataData data);

		const std::string& getTitle() const;
		const std::string& getArtist() const;
		const std::string& getAlbum() const;
		const std::string& getAlbumArtist() const;
		const std::string& getGenre() const;
		const std::string& getYear() const;
		int getTrackNumber() const;
		int getDiscNumber() const;
		const std::string& getComposer() const;
		const std::string& getComment() const;
		const std::string& getDescription() const;
		const std::string& getLyrics() const;
		const std::vector<std::uint8_t>& getCoverArt() const;
		const std::string& getCoverArtMimeType() const;

	private:
		std::string title;
		std::string artist;
		std::string album;
		std::string albumArtist;
		std::string genre;
		std::string year;
		int trackNumber = 0;
		int discNumber = 0;
		std::string composer;
		std::string comment;
		std::string description;
		std::string lyrics;
		std::string coverArtMimeType;
		std::vector<std::uint8_t> coverArt;
	};

	AudioMetadata ReadAudioMetadata(const std::filesystem::path& path);
}
