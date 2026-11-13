# ADR 0005: Add library cover-art and lyrics views

- Status: Accepted
- Date: 2026-11-13

## Context

Audio metadata now provides embedded cover art and lyrics, and the library needs a way to present those fields without changing playback or file-data APIs.

## Decision

Keep two library display modes in the presentation layer: title-only, and title with embedded cover art. Decode cover art lazily when a visible row needs it, use a bounded Direct3D texture cache, and show a neutral placeholder for tracks without usable art. Add a lyrics toggle that opens a left-side panel for the currently selected playback track, with a clear empty state when lyrics or a current track are unavailable.

## Consequences

- The default library view remains compact and title-only; the cover-art view is opt-in for each run.
- Image decoding and GPU texture lifetime stay in `Scripts/Presentation/` and do not expand `MusicFile` responsibilities.
- This presentation implementation follows the existing Windows Direct3D backend; a future platform backend will need its own image-to-texture path.
