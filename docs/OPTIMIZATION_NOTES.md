# Optimization notes — 2026-10-13

These measurements cover isolated CPU work on the Windows i686 Clang `-O2`
build. They do not establish a game frame-rate improvement. The published
The retained changes below shipped in `v0.2.0-reintegrated.3`; subsequent local work is documented separately.

## Retained changes

| Change | Evidence | Preservation checks |
| --- | --- | --- |
| Resolve the immutable language FONT record once per successful configuration | 500,000 lookups: approximately 7.4 ms binary search versus 0.6–0.7 ms cached lookup | Reconfigure, close, failed/missing-FONT pack, independent inspection and unsupported glyphs |
| Skip the no-op upload pass when both VRAM mask options are disabled | Exact RGB conversion helper: approximately 39–42% less CPU time in the isolated upload benchmark | All 65,536 RGB5551 inputs in all four mask combinations; unchanged upload synchronization |
| Reuse consecutive identical texture-page/CLUT checks within one batch | Four-vertex repetition pattern: approximately 64% less scan time; rectangle checks reduced from 16,384 to 4,096 | 10,000 randomized decisions, unique-key case, and pending GPU CLUT-write fixture |
| Return immediately from sprite-snapshot lookup when its queue is empty | Five million empty lookups: approximately 71 ms versus 4 ms | Full queue, replacement, overflow fallback, consumption, reuse and detach lifetimes |
| Share synchronous presentation/font vertex scratch storage | Removes 391,176 bytes (about 382 KiB) of duplicate static storage | 336 generated vertex/clip-stream comparisons and font/title/scanline interleaving |
| Snapshot pure GTE projection controls once per multi-vertex capture | Complete capture/lookup benchmark: about 15–49% less CPU time depending on batch size and clipping | 12,000 varied cases matched flags and float bytes; all GTE controls unchanged; no cross-call cache |
| Compact captured RGBA pixels to RGB in the same buffer | Removes the separate RGB allocation during conversion; optional shrinking retains a valid buffer on failure | 1,025 sizes, allocation/download/shrink failure cases, all RGB555 colors, mask bits and native/VRAM-dump parity |
| Blit presented captures directly into their final RGB buffer | At 2560×1440 removes 11,059,200 temporary pixel bytes and the final row copy (about 10.55 MiB) | 588 nearest/linear comparisons, odd-width pitches, allocation/wrapper/blit failure cleanup and preserved zero initialization |

Capture shrinking can move the allocation and temporarily reach the former
allocation peak. This is a single-buffer conversion improvement, not a guaranteed
process peak-memory reduction. The earlier overnight atlas-span shrink is a
separate change that saves about 5.07 MiB of retained CPU allocation.
The capture improvements apply to the program's capture/debug APIs, not external
OS screen recording software.

## Rejected or deferred

- Generation-stamped geometry tables improved sparse resets but regressed dense
  capture/lookup work by about 13.3% and increased table storage by 16.7%.
- SDL checkpoint CRC matched the existing format but was slower in this build.
- The diagnostic timing-window scan measured about 0.123 microseconds per call;
  replacing it was not justified.
- Mod-file miss caching would change live replacement behavior. Replay identity
  hashes and importer provenance checks remain intact.
- Further floor/shadow ordering changes still require a specific affected mesh
  or effect. Existing primitive shadows already retain original ordering.

No simulation clock, RNG, collision, actor state machine, input sampling or
existing analog-stick-to-D-pad behavior was changed by these optimizations.
Specific speedrun glitches have not been exhaustively replayed. No live game was
launched during this batch.

The combined candidate passes 16 distinct selected checks covering native/scaled GPU
rendering, sharp text, capture output, projection metadata, language packs,
controller packets/hints, scanner fairness and overlay-state layout.

## Local follow-up (13 October, evening)

Interface glyph registration now scans only the previously occupied prefix of
its 1,024-slot queue, resetting that prefix when the queue drains. It retains
hole reuse, replacement, immutable palettes, unsupported-glyph removal and
original overflow fallback. An isolated paired CPU benchmark measured 464.6 ms
versus 421.2 ms for 4,000 boards of 32 glyphs (9.4% less time). At full capacity,
1,000 boards measured 3,775.8 ms versus 3,850.8 ms (2.0% more time). This is a
small-menu tradeoff, not an FPS improvement claim. 200,000 randomized operations
matched original glyph/palette/UV results; the focused CPU fixture passed.

Build scripts derive their default version from CMakeLists.txt; packaging derives
it from the selected build's CMakeCache.txt, retaining explicit overrides and
executable-version validation. This removes the stale .1 default. Default-version
packaging of the .3 build passed. The status document's remaining legacy byte was
normalized to UTF-8. No gameplay or GPU stress tests were run in this follow-up.
