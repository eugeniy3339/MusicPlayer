# ADR 0003: Use dynamic Unicode font fallbacks

- Status: Accepted
- Date: 2026-11-13

## Context

The embedded Dear ImGui font does not contain glyphs for many music filenames, including Cyrillic and CJK scripts. Dear ImGui 1.93 and the current DirectX 11 renderer support dynamic font texture updates.

## Decision

Use Segoe UI as the UI font when available, then merge installed Arial/Tahoma, Japanese, and Chinese fonts as fallback sources. Let Dear ImGui dynamically load glyphs as text is rendered instead of preloading full Unicode ranges. Keep font files system-provided rather than bundling Windows fonts.

## Consequences

- Unicode glyph coverage depends on the relevant Windows fonts being installed.
- Arabic glyphs can be present, but Dear ImGui does not provide bidirectional layout or Arabic shaping; right-to-left strings may not display correctly.
- Dynamic loading avoids building a large atlas for CJK characters that the user never encounters.
