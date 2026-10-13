# RE:INTEGRAL roadmap

## Reintegrated next-release priorities

- Mantis constructor crash corrected from matching executable/symbol evidence;
  the player cleared the encounter and confirmed physical controller hot swapping.
- Preserve Mantis's original statue fallback and Codec hints. Keep controller
  slots stable: a verified different controller can use port 2; same or uncertain
  reconnects retain original behavior. F2/L3+R3 remains an explicit full port-switch
  fallback. Preserve boss phases and the original port-1 damage rules.
- Overnight candidate: accept same-controller USB socket changes only with a
  matching serial and verified Windows physical USB topology. Same-socket
  reconnect and ambiguous/Bluetooth/XInput evidence retain existing fallbacks.
  Physical socket-movement validation is pending; do not infer it from policy tests.
- After the Mantis crash is fixed, add an optional Installed games memory mode
  enabled by default for new launcher profiles, with an opt-out. Preserve saved
  explicit choices. Scan only file/folder names with bounded depth and work limits,
  case-insensitive title aliases and local deduplication; keep off/random/seed.
  Verify Integral's actual dialogue flags. Other Life Azure Dreams is one title
  trigger, not a separate fifth game. Discover configured Steam library paths
  from their configuration without reading game, manifest or save contents.
  Cover the requested standard Windows/Linux roots and accessible WSL mounts.
  Implemented; reserve fair shares of the bounded scan budget for every root
  and nested names. Player launch confirmed Suikoden/Castlevania profile bits.
- Track remaining intermittent floor/lighting texture flicker as longer-term
  renderer work. The player has confirmed the specific Otacon desk-floor fix;
  identify additional affected draws before extending ordering exceptions.
  Code audit confirms primitive shadows already retain original ordering;
  further exceptions require a specific affected mesh/effect identity.
- Finish accepting and consolidating the display, Codec, HUD and mission-log
  candidate fixes while preserving the published package as rollback.
- Pilot optional faithful high-resolution fonts and inventory icons: compare
  original pixels, conservative raster upscaling and cleaned vector outlines
  before selecting a runtime integration path. Retain original asset rendering.
- Build a local asset importer for user-owned **Metal Gear Solid: Master
  Collection Vol. 1**, starting with the Steam installation's Integral content.
  Detect the installation and edition, identify archive/resource revisions,
  extract supported assets into a separate local directory, and report whether
  they are compatible with this runtime. Record provenance and missing resources;
  do not redistribute game files or claim extraction alone enables gameplay.
- Extend that importer to other Master Collection platforms and other owned
  editions where their accessible file formats and asset compatibility are
  understood. Track each source platform separately; Steam support does not
  establish support for console packages. Preserve source installations and
  existing disc-based launching. No DRM or platform-security bypass is planned.

The sections below retain the upstream roadmap as historical project context.

This roadmap separates delivered foundations from planned work. Future items
have no committed delivery dates. The native main campaign is the starting
point; the alpha's supported configuration is in [ALPHA.md](ALPHA.md).

## Native alpha foundations

- [x] Official-source prerequisite setup and a documented Debug build.
- [x] A downloadable Windows build with a shared setup/settings launcher,
  including gameplay options and additional command-line arguments.
- [x] Saves and settings outside the installation folder, plus versioned logs.
- [ ] Broader testing of campaign branches, controllers, graphics hardware and
  display settings, with fixes driven by reported defects.

## Expand language and edition support

- Support text and voice resources from additional regions and editions.
- Identify supported disc revisions and explain compatibility during setup.
- Offer language selection where the supplied resources support it, while
  keeping scripts, timing, subtitles and spoken dialogue consistent.

## Develop the modern renderer

- Validate the [experimental renderer](../renderers/modern/README.md), which now
  provides perspective-correct textures, subpixel model geometry and per-pixel
  depth testing. Extend coverage to remaining effects and campaign scenes.
- Extend its geometry interface toward meshes and materials while preserving
  recovered gameplay behavior and the original renderer as a comparison.
- Support custom graphics and resource mods through clear renderer boundaries.
- Improve rendering only where its behavior and compatibility can be tested.

The Godot prototype is currently paused and is not part of the native alpha.
The existing renderer boundary is a source-level interface, not a stable
binary plugin API or a completed modern mesh/material pipeline.

## Build practical modding tools

- Extend the current exact-length resource overrides to common image, model
  and audio formats, including resized replacements.
- Make resource inspection, export, replacement and validation easier.
- Develop a map/stage editor after the underlying resource and renderer
  interfaces are ready.

## Broaden game and platform coverage

- Recover and validate VR Missions and other content outside the current
  main-campaign scope.
- Keep host-specific code isolated so future platforms remain possible.
  Windows x86 is the supported initial target; x64 and other operating systems
  need separate implementation and validation. The x64 port is deferred, with
  its feasibility audit retained locally.

Track defects and proposed improvements in the
[issue tracker](https://github.com/Drommedhar/mgs-reintegral/issues).

- Next release: investigate remaining camera-item behavior (user reports it is wonky/broken despite corrected widescreen surround).
