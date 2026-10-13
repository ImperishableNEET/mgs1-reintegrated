# Optional high-resolution text pilot

Status: an optional English text runtime candidate uses TeX Gyre Heros Regular,
a Helvetica-compatible face, with a locally generated 8x coverage atlas.
Actual Helvetica was not found locally. The Graphics tab's **Sharp English text**
option and `Play.ps1 -SharpCodecFont` are off by default. Original character
advances, wrapping, centering, line spacing and dialogue timing are retained.
Unsupported messages fall back as a complete board to the original font.

The atlas is loaded from `fonts/texgyreheros_regular_ascii8x_coverage.bmp`
beside the executable. Its GUST Font License and source attribution accompany
the private candidate. No commercial Helvetica font or game glyph bitmap is
bundled. English Codec dialogue, contact names, cutscene captions and location labels are replaced;
the current unpublished candidate additionally replaces tagged block menu lettering.
HUD and icons retain the original assets. Heros is a stylistic candidate, not an exact font match.

Modern launchers now use display-resolution rendering throughout gameplay,
pause, captions and Codec, capped by the renderer's existing 8x limit. The former
forced original-resolution launch policy caused a visible jump from 2x gameplay
to sharp-text rendering, followed by a drop 30 drawn frames after text cleared.
Original-resolution gameplay is now an explicit Graphics preference (or
`Play.ps1 -OriginalResolution`). In that lower-GPU mode, sharp text can still
temporarily raise the resolution. Changes wait for the first drawing frame;
held scaled framebuffer content is preserved directly during target changes.
An independent font target remains a future improvement.

Focused validation covers immutable layout snapshots, antialiasing, PS1 mask
behavior, unsupported/untagged fallback and order-table occlusion at 2x/8x.
A bounded actual-game Codec capture also confirms the replacement is reached.
Full conversation and performance playtesting remain separate from these checks.

The previous private reconstructed specimen covers A–Z and digits but is not
used by this runtime candidate.
The first filtered and literal-contour previews were rejected as too blurry or
pixelated. The next specimen reconstructs English letter shapes from the original
glyphs, retaining descriptor widths, advances, vertical offsets and margins.
The font's intermediate shades represent coverage/intensity; they are not a
separate shadow layer. Preserve Codec's original spacing and 18-pixel line pitch.

## Bounded implementation

Start with English Codec dialogue. The existing font code determines layout and
uploads a bitmap; `draw_radio_message` submits that board as one sprite. Record
glyph placements during the existing layout, freeze them when the bitmap updates,
and explicitly associate that snapshot with the submitted sprite. Render an
authored high-resolution atlas at the sprite's original order-table position.
Do not alter wrapping, centering, dialogue timing or game logic.

Keep the original complete sprite for messages containing unsupported glyphs.
Reset the sidecar on font clear and preserve snapshot lifetime through deferred
renderer submission. Avoid replacing shared VRAM or identifying text by UV/CLUT
alone. The language-pack glyph override currently excludes ASCII.

Judge the specimen at normal display size, then check one ordinary conversation,
turbo dialogue and an aspect transition after integration. Preserve original
font mode as a fallback. Icons are a separate silhouette-reconstruction pilot.

## Reference practices

- [FontForge bitmap-font guidance](https://fontforge.org/docs/tutorial/editexample8.html):
  automatic tracing is unreliable for very small bitmap sources.
- [FreeType glyph metrics](https://freetype.org/freetype2/docs/glyphs/glyphs-3.html):
  bearings, advances, grid fitting and final pixel placement require separate care.
- [MSDFgen](https://github.com/Chlumsky/msdfgen): an optional scalable representation
  for good outlines; it does not reconstruct source detail.
- [Depixelizing Pixel Art](https://johanneskopf.de/publications/pixelart/):
  connectivity-aware reconstruction is worth assessing for small item silhouettes.

Private source-derived specimens remain outside public payloads. A visual
specimen is not evidence that a replacement works in the game renderer.

## Asset-tool reference

[Arms Depot / tools-mgs](https://github.com/Joy-Division/tools-mgs) provides
MGS1 STAGE.DIR, PSX DAR and FACE.DAT extractors plus an incomplete GCX decoder.
Evaluate its format handling as a reference for importer validation and portrait
asset inspection. The archived project documents limited input checking and
16-bit filename-hash collisions; retain stage-scoped identities and validate
bounds before reusing code. Its README does not establish Master Collection
container support or a font reconstruction solution.

## Other font candidates

- Credits, ordinary captions and location names: compare the existing Heros
  face against each original. `menu/jimaku.c` uses the same KCB bitmap-font
  system for ordinary captions/location labels. Credits need their own resource
  audit before assuming identical metrics or weight.
- PAUSE and large interface lettering: [Mission Genome Sans by V. Sarela]
  (https://www.fontstruct.com/fontstructions/show/1968440/mission-genome-sans)
  is explicitly a non-pixelated MGS interface recreation, not Konami's original
  source typeface. Its listed license links to CC BY-NC-ND 3.0; do not treat it
  as a freely modifiable font. The download endpoint returned a sign-in page.
- Small LIFE/item/interface and ammunition numerals: [Gleeson's MGS1 Fonts]
  (https://gleeson.itch.io/mgs1-fonts) contains HUD (5x5), wide menu/Codec (8x6)
  and Ammo (5x7) recreations under CC BY-SA 4.0. These deliberately retain
  bitmap shapes, so increasing their size alone does not smooth stair steps.
- PAUSE uses `_menu_number_draw_string2` in `menu/jimaku.c`; the location label
  uses KCB text plus four offset black sprite copies for its outline. Preserve
  that outline and original placement when adding high-resolution rendering.

These references establish useful recreations; none establishes the original
commercial source face for the custom blocky HUD alphabet.

Codec contact names retain their original row crops, green tint and selected-row additive glow. Frequency digits retain the original assets. Opening credits use pre-rendered texture images and are not covered by this text bridge.

Save/load slot names and bitmap-font status messages also use the sharp-text bridge, with separate 128x14 row crops.
The new interface bridge covers block headings and numerals drawn through
`_menu_number_draw_string2`; other number/HUD paths and selection highlight geometry remain original.

## Interface lettering candidate, 2026-10-13

The existing optional Sharp English text setting now enables a second atlas,
`fonts/interface-font.bmp`, when available. The legacy setting key is retained,
and older packages without this atlas still launch with the ordinary Heros bridge.
PAUSE, save/load block headings and Codec PTT/MEMORY labels are covered through
explicit packet metadata at the native 8x6 glyph emitter. Original final packet
positions preserve centering, narrow-I spacing, format spacing and line pitch.
Unsupported symbols, unknown source panels, palettes using additional ink colors,
semitransparent packets and missing atlases retain the original rendering.

The atlas derives from [Andrew Gleeson's MGS1 font recreation](https://gleeson.itch.io/mgs1-fonts),
under CC BY-SA 4.0. The supplied source TTF, attribution and license accompany it.
It is a recreation rather than evidence of Konami's original source font.
Private comparison verified all 26 letters and 10 numerals against the original
binary 8x6 ink after correcting numeral 1's one-pixel bearing. The faithful
alternative retains those outlines. The selected candidate straightens clear
diagonal staircases in K/M/N/W/X/Y/Z while preserving outline bounds, advances,
square stems and corners. Unmatched slash and other special symbols remain original.

Reproduce without game assets: `python tools/build_interface_font.py --smooth --output interface-font.bmp`.
Pillow is required; smoothing additionally needs fontTools. The JSON manifest
records changes, glyph bounds and source/atlas hashes. These derived font assets
remain CC BY-SA 4.0; the font license does not change the engine's license scope.
The package includes an unsmoothed `interface-font-original.bmp` alternative;
copy it over `interface-font.bmp` to select that outline treatment.

The source palette is captured at upload and frozen per submitted glyph,
preserving native half-bright ink and the PS1 mask bit. Coverage chunks now batch
glyphs into a common viewport; original glyph order, colors, clipping and masks
are retained. Focused 2x/8x GPU fixtures include a mixed-color board exceeding
one chunk, native palette/mask behavior, row crops, fallback and order-table
occlusion. No full game-menu playtest was performed for this batch.

## Opening-credit resource audit (2026-10-13)

Opening-credit lettering is authored as PCX texture images, not as Jimaku KCB
text. `source/takabe/telop.c` creates the `DEMO_TEXT` / `DEMO_TEXT2` actors
(`source/include/charalst.h`, IDs 0x0005 / 0x004c). Its constructor receives
texture hashes (or texture pointers), and `telop_800DD550` creates two SPRT
passes: subtractive ABR 2 (tpage 0x40) followed by additive ABR 1 (0x20).
Both packets are merged into the original order table. `telop_800DD730` applies
the authored fade while leaving actor timing untouched.

Private inspection of the owned Integral Disc 1 `d00a` PCX resources verifies
these label identities. These are stage-scoped metadata, not extracted artwork:

| Texture hash | Visible wording | Board dimensions |
| --- | --- | --- |
| d96a | Motoyuki Yoshioka | 136 x 16 |
| 76ca | Production/Copyright | 164 x 16 |
| ce37 | KCE Japan Co.,Ltd. | 136 x 16 |
| fde4 | Artwork directed by | 136 x 16 |
| 46b2 | Frederick Bloggs | 124 x 16 |
| ed3e | Christopher Fritz | 120 x 16 |
| 150e | Carren Learning | 116 x 16 |
| d7ad | Konami Co.,Ltd. | 116 x 16 |
| c01f | Renne Collette | 108 x 16 |
| cf19 | Yoji Shinkawa | 100 x 16 |
| fd76 / cf2d | George Byrd | 92 x 16 |
| c459 | James Flinders | 108 x 14 |
| 52e0 | Hideo Kojima | 92 x 16 |
| 8c09 | Kim Nguyen | 88 x 16 |
| a70a | David Hayter | 96 x 16 |
| b8cf | Chuck Farley | 96 x 16 |
| c182 | Doug Stone | 88 x 16 |
| 4508 | Mae Zadler | 84 x 16 |
| 4814 | Bert Stewart | 96 x 16 |
| 450d | Patric Laine | 88 x 16 |
| 53a4 | Julie Monroe | 96 x 16 |
| 7a4c | Directed by | 84 x 16 |
| 92ce | Developed by | 84 x 16 |
| 603a | Produced by | 96 x 16 |
| e4fc | Paul Otis | 64 x 12 |

A future faithful replacement should use constructor identity plus verified stage,
board dimensions and original ink bounds, then associate each exact submitted
packet with a line-specific authored Heros coverage asset. TeX Gyre Heros remains
a comparison candidate, not a verified identification of Konami's source font.
Do not reuse Codec `font.res` character advances for these proportional credit
lines: source board padding, ink width and baseline differ. Preserve the existing
subtractive and additive passes, shade, positions, merged-packet ordering and
original fallback for unknown hashes/pointer-form constructors. UV coordinates
alone are insufficient because unrelated textures share atlas locations.

No opening-credit replacement is enabled by this audit. The separate ending
credits use compressed `credit.r` boards through `source/takabe/ending2.c` and
`ed_telop.c`, and require a separate text/resource audit. The game logo and other
non-text graphics remain outside this proposed replacement.

## Coverage-batch validation

The coverage renderer batches glyph spans within 112 x 48 logical-pixel bands.
The solid-color shader clips to a 128 x 64 logical-pixel coordinate domain at
8x atlas scale; band margins retain the complete 16 x 14 glyph cell. Changing
bands flushes the previous viewport first, preserving glyph overlap and mask
ordering. A single board-wide viewport was rejected because distant glyphs
fell outside the shader's clip domain.

Using the shipped Heros coverage spans and original ASCII advances, the pictured
four-line crouch message predicts 98 to 41 uploads (196 to 82 body/mask draws);
the two-line Watergate message predicts 36 to 16 uploads, and the Alaska caption
24 to 10. These are layout-specific command-count models, not measured FPS or
universal reductions. Source-row crops, packet ordering, color modulation and
PS1 mask writes remain separate from that batching.
