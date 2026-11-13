# ADR 0004: Extract common metadata when creating MusicFile

- Status: Accepted
- Date: 2026-11-13

## Context

The library supports WAV, MP3, and FLAC files and needs common tags and embedded cover art available to the rest of the app without reading the audio file again.

## Decision

Extract metadata in the `MusicFile` constructor using a small internal reader. Support RIFF INFO and ID3 chunks in WAV, ID3v2.3/v2.4 and ID3v1 tags in MP3, and Vorbis comments and PICTURE blocks in FLAC. Store text as UTF-8 and cover art as original image bytes plus MIME type. Keep `MusicFile` limited to its path, display name, and an `AudioMetadata` value; expose tag getters on `AudioMetadata` and use the filename as the display-title fallback.

## Consequences

- No external tagging library is added to the project.
- The reader bounds metadata and cover-art allocations to limit memory use on malformed files.
- Other containers and uncommon ID3 encodings/features are not covered; the reader can be replaced by a dedicated tagging library if broader format support is needed.
