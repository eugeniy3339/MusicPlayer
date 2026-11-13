#include "AudioMetadataReader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <utility>

namespace MusicPlayer {
	namespace {
		constexpr std::size_t maxMetadataBlockSize = 64 * 1024 * 1024;
		constexpr std::size_t maxCoverArtSize = 16 * 1024 * 1024;

		std::uint32_t ReadBigEndian32(const std::uint8_t* data) {
			return (static_cast<std::uint32_t>(data[0]) << 24) |
				(static_cast<std::uint32_t>(data[1]) << 16) |
				(static_cast<std::uint32_t>(data[2]) << 8) |
				static_cast<std::uint32_t>(data[3]);
		}

		std::uint32_t ReadLittleEndian32(const std::uint8_t* data) {
			return static_cast<std::uint32_t>(data[0]) |
				(static_cast<std::uint32_t>(data[1]) << 8) |
				(static_cast<std::uint32_t>(data[2]) << 16) |
				(static_cast<std::uint32_t>(data[3]) << 24);
		}

		std::uint32_t ReadSynchsafe32(const std::uint8_t* data) {
			return (static_cast<std::uint32_t>(data[0] & 0x7f) << 21) |
				(static_cast<std::uint32_t>(data[1] & 0x7f) << 14) |
				(static_cast<std::uint32_t>(data[2] & 0x7f) << 7) |
				static_cast<std::uint32_t>(data[3] & 0x7f);
		}

		void AppendUtf8(std::string& output, std::uint32_t codePoint) {
			if (codePoint <= 0x7f) {
				output.push_back(static_cast<char>(codePoint));
			}
			else if (codePoint <= 0x7ff) {
				output.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
				output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
			}
			else if (codePoint <= 0xffff) {
				output.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
				output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
				output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
			}
			else if (codePoint <= 0x10ffff) {
				output.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
				output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
				output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
				output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
			}
		}

		std::string DecodeText(const std::uint8_t* data, std::size_t size, std::uint8_t encoding) {
			if (encoding == 1 || encoding == 2) {
				while (size >= 2 && data[size - 1] == 0 && data[size - 2] == 0)
					size -= 2;
			}
			else {
				while (size > 0 && data[size - 1] == 0)
					--size;
			}
			std::string output;
			if (encoding == 3) {
				for (std::size_t index = 0; index < size; ++index) {
					if (data[index] == 0) output += "; ";
					else output.push_back(static_cast<char>(data[index]));
				}
				return output;
			}
			if (encoding == 1 || encoding == 2) {
				bool littleEndian = encoding == 1;
				std::size_t index = 0;
				if (size >= 2 && data[0] == 0xfe && data[1] == 0xff) {
					littleEndian = false;
					index = 2;
				}
				else if (size >= 2 && data[0] == 0xff && data[1] == 0xfe) {
					littleEndian = true;
					index = 2;
				}
				while (index + 1 < size) {
					std::uint32_t codePoint = littleEndian
						? static_cast<std::uint32_t>(data[index] | (data[index + 1] << 8))
						: static_cast<std::uint32_t>((data[index] << 8) | data[index + 1]);
					index += 2;
					if (codePoint >= 0xd800 && codePoint <= 0xdbff && index + 1 < size) {
						const std::uint32_t low = littleEndian
							? static_cast<std::uint32_t>(data[index] | (data[index + 1] << 8))
							: static_cast<std::uint32_t>((data[index] << 8) | data[index + 1]);
						if (low >= 0xdc00 && low <= 0xdfff) {
							codePoint = 0x10000 + ((codePoint - 0xd800) << 10) + (low - 0xdc00);
							index += 2;
						}
					}
					if (codePoint == 0)
						output += "; ";
					else
						AppendUtf8(output, codePoint);
				}
				return output;
			}

			for (std::size_t index = 0; index < size; ++index) {
				if (data[index] == 0) output += "; ";
				else AppendUtf8(output, data[index]); // ID3 encoding 0 is ISO-8859-1.
			}
			return output;
		}

		std::string DecodeVorbisText(const std::uint8_t* data, std::size_t size) {
			return DecodeText(data, size, 3);
		}

		std::string Uppercase(std::string value) {
			for (char& character : value)
				character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			return value;
		}

		int ParseIndex(const std::string& value) {
			int result = 0;
			bool foundDigit = false;
			for (char character : value) {
				if (character < '0' || character > '9') {
					if (foundDigit) break;
					continue;
			}
				foundDigit = true;
				if (result > (std::numeric_limits<int>::max() - 9) / 10)
					return 0;
				result = result * 10 + (character - '0');
			}
			return result;
		}

		void SetComment(AudioMetadataData& metadata, const std::string& value) {
			if (metadata.comment.empty()) metadata.comment = value;
		}

		void SetDescription(AudioMetadataData& metadata, const std::string& value) {
			if (metadata.description.empty()) metadata.description = value;
		}

		void SetTag(AudioMetadataData& metadata, const std::string& key, const std::string& value) {
			if (value.empty()) return;
			const std::string normalized = Uppercase(key);
			if (normalized == "TITLE" && metadata.title.empty()) metadata.title = value;
			else if (normalized == "ARTIST" && metadata.artist.empty()) metadata.artist = value;
			else if ((normalized == "ALBUMARTIST" || normalized == "ALBUM ARTIST") && metadata.albumArtist.empty()) metadata.albumArtist = value;
			else if (normalized == "ALBUM" && metadata.album.empty()) metadata.album = value;
			else if (normalized == "GENRE" && metadata.genre.empty()) metadata.genre = value;
			else if ((normalized == "DATE" || normalized == "YEAR" || normalized == "TDRC" || normalized == "TYER") && metadata.year.empty()) metadata.year = value;
			else if ((normalized == "TRACKNUMBER" || normalized == "TRACK" || normalized == "TRCK") && metadata.trackNumber == 0) metadata.trackNumber = ParseIndex(value);
			else if ((normalized == "DISCNUMBER" || normalized == "DISC" || normalized == "TPOS") && metadata.discNumber == 0) metadata.discNumber = ParseIndex(value);
			else if (normalized == "COMPOSER" && metadata.composer.empty()) metadata.composer = value;
			else if ((normalized == "DESCRIPTION" || normalized == "SUBTITLE") && metadata.description.empty()) metadata.description = value;
			else if ((normalized == "COMMENT" || normalized == "COMMENTS") && metadata.comment.empty()) metadata.comment = value;
			else if ((normalized == "LYRICS" || normalized == "UNSYNCEDLYRICS") && metadata.lyrics.empty()) metadata.lyrics = value;
		}

		std::string DecodeEncodedString(const std::uint8_t* data, std::size_t size, std::size_t& offset,
			std::uint8_t encoding) {
			const std::size_t start = offset;
			const bool utf16 = encoding == 1 || encoding == 2;
			while (offset < size) {
				if (!utf16 && data[offset] == 0) break;
				if (utf16 && offset + 1 < size && data[offset] == 0 && data[offset + 1] == 0) break;
				offset += utf16 ? 2 : 1;
			}
			const std::string value = DecodeText(data + start, offset - start, encoding);
			if (offset < size) offset += utf16 ? 2 : 1;
			return value;
		}

		bool ReadAt(std::ifstream& stream, std::uint64_t offset, std::size_t size,
			std::vector<std::uint8_t>& output, std::size_t maximum = maxMetadataBlockSize) {
			if (size > maximum || offset > static_cast<std::uint64_t>((std::numeric_limits<std::streamoff>::max)()))
				return false;
			output.resize(size);
			stream.clear();
			stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
			if (!stream || !stream.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(size))) {
				output.clear();
				return false;
			}
			return true;
		}

		std::uint32_t ReadSynchsafeValue(const std::uint8_t* data, std::size_t size) {
			std::uint32_t value = 0;
			for (std::size_t index = 0; index < size; ++index)
				value = (value << 7) | (data[index] & 0x7f);
			return value;
		}

		void ParseId3Picture(AudioMetadataData& metadata, const std::uint8_t* data, std::size_t size) {
			if (size < 5 || !metadata.coverArt.empty()) return;
			const std::uint8_t encoding = data[0];
			std::size_t mimeEnd = 1;
			while (mimeEnd < size && data[mimeEnd] != 0) ++mimeEnd;
			if (mimeEnd >= size || mimeEnd + 1 >= size) return;
			const std::string mime(reinterpret_cast<const char*>(data + 1), mimeEnd - 1);
			std::size_t descriptionOffset = mimeEnd + 2; // Skip MIME terminator and picture type.
			DecodeEncodedString(data, size, descriptionOffset, encoding);
			if (descriptionOffset >= size || size - descriptionOffset > maxCoverArtSize) return;
			metadata.coverArtMimeType = mime.empty() || mime == "-->" ? "application/octet-stream" : mime;
			metadata.coverArt.assign(data + descriptionOffset, data + size);
		}

		void ParseId3Frame(AudioMetadataData& metadata, const std::string& frameId,
			const std::uint8_t* data, std::size_t size) {
			if (size == 0) return;
			if (frameId == "APIC") {
				ParseId3Picture(metadata, data, size);
				return;
			}
			const std::uint8_t encoding = data[0];
			if (frameId == "COMM" || frameId == "USLT") {
				if (size < 4) return;
				std::size_t offset = 4; // Encoding byte and three-byte language.
				const std::string description = DecodeEncodedString(data, size, offset, encoding);
				const std::string text = DecodeText(data + offset, size - offset, encoding);
				if (frameId == "COMM") {
					SetComment(metadata, text);
					if (description == "DESCRIPTION") SetDescription(metadata, text);
				}
				else if (metadata.lyrics.empty()) metadata.lyrics = text;
				return;
			}

			const std::string value = DecodeText(data + 1, size - 1, encoding);
			if (frameId == "TIT2") SetTag(metadata, "TITLE", value);
			else if (frameId == "TPE1") SetTag(metadata, "ARTIST", value);
			else if (frameId == "TPE2") SetTag(metadata, "ALBUMARTIST", value);
			else if (frameId == "TALB") SetTag(metadata, "ALBUM", value);
			else if (frameId == "TCON") SetTag(metadata, "GENRE", value);
			else if (frameId == "TDRC" || frameId == "TYER") SetTag(metadata, "YEAR", value);
			else if (frameId == "TRCK") SetTag(metadata, "TRACKNUMBER", value);
			else if (frameId == "TPOS") SetTag(metadata, "DISCNUMBER", value);
			else if (frameId == "TCOM") SetTag(metadata, "COMPOSER", value);
			else if (frameId == "TIT3") SetTag(metadata, "DESCRIPTION", value);
		}

		void ParseId3Tag(AudioMetadataData& metadata, const std::vector<std::uint8_t>& bytes) {
			if (bytes.size() < 10 || bytes[0] != 'I' || bytes[1] != 'D' || bytes[2] != '3') return;
			const std::uint8_t version = bytes[3];
			if (version < 3 || version > 4 || (bytes[5] & 0x80) != 0) return; // Skip compressed/unsynchronized tags.
			const std::size_t tagSize = ReadSynchsafe32(bytes.data() + 6);
			if (tagSize > bytes.size() - 10) return;
			std::size_t offset = 10;
			const std::size_t end = 10 + tagSize;
			if ((bytes[5] & 0x40) != 0 && offset + 4 <= end) {
				const std::uint32_t extendedSize = version == 4
					? ReadSynchsafe32(bytes.data() + offset)
					: ReadBigEndian32(bytes.data() + offset);
				offset += version == 4 ? extendedSize : static_cast<std::size_t>(extendedSize) + 4;
			}
			while (offset + 10 <= end) {
				const std::uint8_t* header = bytes.data() + offset;
				if (header[0] == 0) break;
				const std::string frameId(reinterpret_cast<const char*>(header), 4);
				const std::uint32_t frameSize = version == 4
					? ReadSynchsafe32(header + 4)
					: ReadBigEndian32(header + 4);
				if (frameSize == 0 || frameSize > end - offset - 10) break;
				const std::uint8_t formatFlags = header[9];
				const bool unsupportedFrame = version == 3
					? (formatFlags & 0xc0) != 0
					: (formatFlags & 0x0c) != 0;
				if (!unsupportedFrame)
					ParseId3Frame(metadata, frameId, header + 10, frameSize);
				offset += 10 + frameSize;
			}
		}

		void ParseId3v1(AudioMetadataData& metadata, const std::vector<std::uint8_t>& bytes) {
			if (bytes.size() != 128 || bytes[0] != 'T' || bytes[1] != 'A' || bytes[2] != 'G') return;
			auto text = [&bytes](std::size_t offset, std::size_t size) {
				while (size > 0 && (bytes[offset + size - 1] == 0 || bytes[offset + size - 1] == ' ')) --size;
				return DecodeText(bytes.data() + offset, size, 0);
			};
			SetTag(metadata, "TITLE", text(3, 30));
			SetTag(metadata, "ARTIST", text(33, 30));
			SetTag(metadata, "ALBUM", text(63, 30));
			SetTag(metadata, "YEAR", text(93, 4));
			if (bytes[125] == 0 && bytes[126] != 0)
				SetTag(metadata, "TRACKNUMBER", std::to_string(bytes[126]));
			else
				SetComment(metadata, text(97, 30));
		}

		void ParseVorbisComments(AudioMetadataData& metadata, const std::vector<std::uint8_t>& block) {
			if (block.size() < 8) return;
			std::size_t offset = 0;
			const std::uint32_t vendorLength = ReadLittleEndian32(block.data() + offset);
			offset += 4;
			if (vendorLength > block.size() - offset) return;
			offset += vendorLength;
			if (offset + 4 > block.size()) return;
			const std::uint32_t count = ReadLittleEndian32(block.data() + offset);
			offset += 4;
			for (std::uint32_t index = 0; index < count && offset + 4 <= block.size(); ++index) {
				const std::uint32_t length = ReadLittleEndian32(block.data() + offset);
				offset += 4;
				if (length > block.size() - offset) return;
				const std::string entry = DecodeVorbisText(block.data() + offset, length);
				offset += length;
				const std::size_t separator = entry.find('=');
				if (separator == std::string::npos) continue;
				const std::string key = entry.substr(0, separator);
				const std::string value = entry.substr(separator + 1);
				if (Uppercase(key) == "METADATA_BLOCK_PICTURE") {
					// FLAC picture comments use base64-encoded FLAC PICTURE blocks.
					static constexpr char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
					std::array<signed char, 256> decode{};
					decode.fill(-1);
					for (int i = 0; i < 64; ++i) decode[static_cast<unsigned char>(base64[i])] = static_cast<signed char>(i);
					std::vector<std::uint8_t> picture;
					unsigned int accumulator = 0;
					int bits = 0;
					for (unsigned char character : value) {
						if (character == '=') break;
						if (decode[character] < 0) continue;
						accumulator = (accumulator << 6) | static_cast<unsigned int>(decode[character]);
						bits += 6;
						if (bits >= 8) {
							bits -= 8;
						picture.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xff));
							if (picture.size() > maxMetadataBlockSize) break;
						}
					}
					if (!picture.empty()) {
						std::size_t pictureOffset = 0;
						auto readLengthString = [&picture, &pictureOffset](std::string& output) {
							if (pictureOffset + 4 > picture.size()) return false;
							const std::uint32_t length = ReadBigEndian32(picture.data() + pictureOffset);
							pictureOffset += 4;
							if (length > picture.size() - pictureOffset) return false;
							output.assign(reinterpret_cast<const char*>(picture.data() + pictureOffset), length);
							pictureOffset += length;
							return true;
						};
						if (picture.size() >= 4) {
							pictureOffset = 4; // Picture type.
							std::string mime;
							if (readLengthString(mime) && readLengthString(metadata.description) && pictureOffset + 20 <= picture.size()) {
								pictureOffset += 16; // Width, height, color depth, and palette entries.
								const std::uint32_t imageSize = ReadBigEndian32(picture.data() + pictureOffset);
								pictureOffset += 4;
								if (imageSize <= picture.size() - pictureOffset && imageSize <= maxCoverArtSize) {
									metadata.coverArtMimeType = mime;
									metadata.coverArt.assign(picture.begin() + pictureOffset, picture.begin() + pictureOffset + imageSize);
								}
							}
						}
					}
				}
				else {
					SetTag(metadata, key, value);
				}
			}
		}

		void ParseFlac(AudioMetadataData& metadata, std::ifstream& stream, std::uint64_t fileSize) {
			std::vector<std::uint8_t> marker;
			if (!ReadAt(stream, 0, 4, marker) || std::string(marker.begin(), marker.end()) != "fLaC") return;
			std::uint64_t offset = 4;
			bool lastBlock = false;
			while (!lastBlock && offset + 4 <= fileSize) {
				std::vector<std::uint8_t> header;
				if (!ReadAt(stream, offset, 4, header, 4)) return;
				lastBlock = (header[0] & 0x80) != 0;
				const std::uint8_t type = header[0] & 0x7f;
				const std::size_t blockSize = (static_cast<std::size_t>(header[1]) << 16) |
					(static_cast<std::size_t>(header[2]) << 8) | header[3];
				offset += 4;
				if (blockSize > fileSize - offset || blockSize > maxMetadataBlockSize) return;
				if (type == 4 || type == 6) {
					std::vector<std::uint8_t> block;
					if (!ReadAt(stream, offset, blockSize, block)) return;
					if (type == 4) ParseVorbisComments(metadata, block);
					else if (metadata.coverArt.empty() && block.size() >= 4) {
						std::size_t pictureOffset = 4; // Picture type.
						auto readLengthString = [&block, &pictureOffset](std::string& output) {
							if (pictureOffset + 4 > block.size()) return false;
							const std::uint32_t length = ReadBigEndian32(block.data() + pictureOffset);
							pictureOffset += 4;
							if (length > block.size() - pictureOffset) return false;
							output.assign(reinterpret_cast<const char*>(block.data() + pictureOffset), length);
							pictureOffset += length;
							return true;
						};
						std::string mime;
						if (readLengthString(mime) && readLengthString(metadata.description) && pictureOffset + 20 <= block.size()) {
							pictureOffset += 16;
							const std::uint32_t imageSize = ReadBigEndian32(block.data() + pictureOffset);
							pictureOffset += 4;
							if (imageSize <= block.size() - pictureOffset && imageSize <= maxCoverArtSize) {
								metadata.coverArtMimeType = mime;
								metadata.coverArt.assign(block.begin() + pictureOffset, block.begin() + pictureOffset + imageSize);
							}
						}
					}
				}
				offset += blockSize;
			}
		}

		void ParseWaveInfo(AudioMetadataData& metadata, const std::vector<std::uint8_t>& block) {
			if (block.size() < 4 || std::string(block.begin(), block.begin() + 4) != "INFO") return;
			std::size_t offset = 4;
			while (offset + 8 <= block.size()) {
				const std::string key(reinterpret_cast<const char*>(block.data() + offset), 4);
				const std::uint32_t length = ReadLittleEndian32(block.data() + offset + 4);
				offset += 8;
				if (length > block.size() - offset) return;
				const std::string value = DecodeText(block.data() + offset, length, 0);
				if (key == "INAM") SetTag(metadata, "TITLE", value);
				else if (key == "IART") SetTag(metadata, "ARTIST", value);
				else if (key == "IPRD") SetTag(metadata, "ALBUM", value);
				else if (key == "IGNR") SetTag(metadata, "GENRE", value);
				else if (key == "ICRD") SetTag(metadata, "YEAR", value);
				else if (key == "ITRK") SetTag(metadata, "TRACKNUMBER", value);
				else if (key == "ICMT") SetComment(metadata, value);
				else if (key == "ISBJ") SetDescription(metadata, value);
				offset += length + (length & 1);
			}
		}

		void ParseWave(AudioMetadataData& metadata, std::ifstream& stream, std::uint64_t fileSize) {
			std::vector<std::uint8_t> header;
			if (!ReadAt(stream, 0, 12, header, 12) || std::string(header.begin(), header.begin() + 4) != "RIFF" ||
				std::string(header.begin() + 8, header.end()) != "WAVE") return;
			std::uint64_t offset = 12;
			while (offset + 8 <= fileSize) {
				std::vector<std::uint8_t> chunkHeader;
				if (!ReadAt(stream, offset, 8, chunkHeader, 8)) return;
				const std::string id(reinterpret_cast<const char*>(chunkHeader.data()), 4);
				const std::uint32_t chunkSize = ReadLittleEndian32(chunkHeader.data() + 4);
				offset += 8;
				if (chunkSize > fileSize - offset) return;
				if ((id == "LIST" || id == "ID3 " || id == "id3 ") && chunkSize <= maxMetadataBlockSize) {
					std::vector<std::uint8_t> block;
					if (!ReadAt(stream, offset, chunkSize, block)) return;
					if (id == "LIST") ParseWaveInfo(metadata, block);
					else ParseId3Tag(metadata, block);
				}
				offset += chunkSize + (chunkSize & 1);
			}
		}

		void ParseMp3(AudioMetadataData& metadata, std::ifstream& stream, std::uint64_t fileSize) {
			std::vector<std::uint8_t> header;
			if (fileSize >= 10 && ReadAt(stream, 0, 10, header, 10) && header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
				const std::size_t tagSize = ReadSynchsafe32(header.data() + 6);
				std::vector<std::uint8_t> tag;
				if (tagSize <= maxMetadataBlockSize && ReadAt(stream, 0, tagSize + 10, tag))
					ParseId3Tag(metadata, tag);
			}
			if (fileSize >= 128) {
				std::vector<std::uint8_t> tag;
				if (ReadAt(stream, fileSize - 128, 128, tag, 128)) ParseId3v1(metadata, tag);
			}
		}
	}

	AudioMetadata ReadAudioMetadata(const std::filesystem::path& path) {
		AudioMetadataData metadata;
		std::ifstream stream(path, std::ios::binary);
		if (!stream) return AudioMetadata();
		std::error_code error;
		const std::uint64_t fileSize = std::filesystem::file_size(path, error);
		if (error || fileSize == 0) return AudioMetadata();
		const std::string extension = path.extension().string();
		if (extension == ".mp3" || extension == ".MP3") ParseMp3(metadata, stream, fileSize);
		else if (extension == ".flac" || extension == ".FLAC") ParseFlac(metadata, stream, fileSize);
		else if (extension == ".wav" || extension == ".WAV") ParseWave(metadata, stream, fileSize);
		return AudioMetadata(std::move(metadata));
	}

	AudioMetadata::AudioMetadata(AudioMetadataData data)
		: title(std::move(data.title)), artist(std::move(data.artist)), album(std::move(data.album)),
		albumArtist(std::move(data.albumArtist)), genre(std::move(data.genre)), year(std::move(data.year)),
		trackNumber(data.trackNumber), discNumber(data.discNumber), composer(std::move(data.composer)),
		comment(std::move(data.comment)), description(std::move(data.description)), lyrics(std::move(data.lyrics)),
		coverArtMimeType(std::move(data.coverArtMimeType)), coverArt(std::move(data.coverArt)) {}

	const std::string& AudioMetadata::getTitle() const { return title; }
	const std::string& AudioMetadata::getArtist() const { return artist; }
	const std::string& AudioMetadata::getAlbum() const { return album; }
	const std::string& AudioMetadata::getAlbumArtist() const { return albumArtist; }
	const std::string& AudioMetadata::getGenre() const { return genre; }
	const std::string& AudioMetadata::getYear() const { return year; }
	int AudioMetadata::getTrackNumber() const { return trackNumber; }
	int AudioMetadata::getDiscNumber() const { return discNumber; }
	const std::string& AudioMetadata::getComposer() const { return composer; }
	const std::string& AudioMetadata::getComment() const { return comment; }
	const std::string& AudioMetadata::getDescription() const { return description; }
	const std::string& AudioMetadata::getLyrics() const { return lyrics; }
	const std::vector<std::uint8_t>& AudioMetadata::getCoverArt() const { return coverArt; }
	const std::string& AudioMetadata::getCoverArtMimeType() const { return coverArtMimeType; }
}
